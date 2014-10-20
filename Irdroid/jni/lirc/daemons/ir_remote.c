/*      $Id: ir_remote.c,v 5.49 2010/05/13 16:24:29 lirc Exp $      */

/****************************************************************************
 ** ir_remote.c *************************************************************
 ****************************************************************************
 *
 * ir_remote.c - sends and decodes the signals from IR remotes
 * 
 * Copyright (C) 1996,97 Ralph Metzler (rjkm@thp.uni-koeln.de)
 * Copyright (C) 1998 Christoph Bartelmus (lirc@bartelmus.de)
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>

#include <sys/ioctl.h>

#include "drivers/lirc.h"

#include "lircd.h"
#include "ir_remote.h"
#include "hardware.h"
#include "release.h"

struct ir_remote *decoding=NULL;

struct ir_remote *last_remote=NULL;
struct ir_remote *repeat_remote=NULL;
struct ir_ncode *repeat_code;

extern struct hardware hw;

static inline lirc_t time_left(struct timeval *current,struct timeval *last,
			lirc_t gap)
{
	unsigned long secs,diff;
	
	secs=current->tv_sec-last->tv_sec;
	
	diff=1000000*secs+current->tv_usec-last->tv_usec;
	
	return((lirc_t) (diff<gap ? gap-diff:0));
}

static int match_ir_code(struct ir_remote *remote, ir_code a, ir_code b)
{
	return ((remote->ignore_mask|a) == (remote->ignore_mask|b) ||
		(remote->ignore_mask|a) == 
		(remote->ignore_mask|(b^remote->toggle_bit_mask)));
}

void get_frequency_range(struct ir_remote *remotes,
			 unsigned int *min_freq,unsigned int *max_freq)
{
	struct ir_remote *scan;
	
	/* use remotes carefully, it may be changed on SIGHUP */
	scan=remotes;
	if(scan==NULL)
	{
		*min_freq=0;
		*max_freq=0;
	}
	else
	{
		*min_freq=scan->freq;
		*max_freq=scan->freq;
		scan=scan->next;
	}
	while(scan)
	{
		if(scan->freq!=0)
		{
			if(scan->freq>*max_freq)
			{
				*max_freq=scan->freq;
			}
			else if(scan->freq<*min_freq)
			{
				*min_freq=scan->freq;
			}
		}
		scan=scan->next;
	}
}

void get_filter_parameters(struct ir_remote *remotes,
			   lirc_t *max_gap_lengthp,
			   lirc_t *min_pulse_lengthp,
			   lirc_t *min_space_lengthp,
			   lirc_t *max_pulse_lengthp,
			   lirc_t *max_space_lengthp)
{
	struct ir_remote *scan = remotes;
	lirc_t max_gap_length = 0;
	lirc_t min_pulse_length = 0, min_space_length = 0;
	lirc_t max_pulse_length = 0, max_space_length = 0;
	
	while(scan)
	{
		lirc_t val;
		val = upper_limit(scan, scan->max_gap_length);
		if(val > max_gap_length)
		{
			max_gap_length = val;
		}
		val = lower_limit(scan, scan->min_pulse_length);
		if(min_pulse_length == 0 || val < min_pulse_length)
		{
			min_pulse_length = val;
		}
		val = lower_limit(scan, scan->min_space_length);
		if(min_space_length == 0 || val > min_space_length)
		{
			min_space_length = val;
		}
		val = upper_limit(scan, scan->max_pulse_length);
		if(val > max_pulse_length)
		{
			max_pulse_length = val;
		}
		val = upper_limit(scan, scan->max_space_length);
		if(val > max_space_length)
		{
			max_space_length = val;
		}
		scan=scan->next;
	}
	*max_gap_lengthp = max_gap_length;
	*min_pulse_lengthp = min_pulse_length;
	*min_space_lengthp = min_space_length;
	*max_pulse_lengthp = max_pulse_length;
	*max_space_lengthp = max_space_length;
}

struct ir_remote *is_in_remotes(struct ir_remote *remotes,
				struct ir_remote *remote)
{
	while(remotes != NULL)
	{
		if(remotes == remote)
		{
			return remote;
		}
		remotes = remotes->next;
	}
	return NULL;
}

struct ir_remote *get_ir_remote(struct ir_remote *remotes,char *name)
{
	struct ir_remote *all;

	/* use remotes carefully, it may be changed on SIGHUP */
	all=remotes;
	while(all)
	{
		if(strcasecmp(all->name,name)==0)
		{
			return(all);
		}
		all=all->next;
	}
	return(NULL);
}

int map_code(struct ir_remote *remote,
	     ir_code *prep,ir_code *codep,ir_code *postp,
	     int pre_bits,ir_code pre,
	     int bits,ir_code code,
	     int post_bits,ir_code post)
{
	ir_code all;
	
	if(pre_bits+bits+post_bits!=
	   remote->pre_data_bits+remote->bits+remote->post_data_bits)
	{
		return(0);
	}
	all=(pre&gen_mask(pre_bits));
	all<<=bits;
	all|=(code&gen_mask(bits));
	all<<=post_bits;
	all|=(post&gen_mask(post_bits));
	
	*postp=(all&gen_mask(remote->post_data_bits));
	all>>=remote->post_data_bits;
	*codep=(all&gen_mask(remote->bits));
	all>>=remote->bits;
	*prep=(all&gen_mask(remote->pre_data_bits));
	
	LOGPRINTF(1,"pre: %llx", (unsigned long long) *prep);
	LOGPRINTF(1,"code: %llx", (unsigned long long) *codep);
	LOGPRINTF(1,"post: %llx", (unsigned long long) *postp);
	LOGPRINTF(1, "code:                   %016llx\n", code);
	
	return(1);
}

void map_gap(struct ir_remote *remote,
	     struct timeval *start, struct timeval *last,
	     lirc_t signal_length,
	     int *repeat_flagp,
	     lirc_t *min_remaining_gapp,
	     lirc_t *max_remaining_gapp)
{
	// Time gap (us) between a keypress on the remote control and
	// the next one.
	lirc_t gap;
	
	// Check the time gap between the last keypress and this one.
	if (start->tv_sec - last->tv_sec >= 2) {
		// Gap of 2 or more seconds: this is not a repeated keypress.
		*repeat_flagp = 0;
		gap = 0;
	} else {
		// Calculate the time gap in microseconds.
		gap = time_elapsed(last, start);
		if(expect_at_most(remote, gap, remote->max_remaining_gap))
		{
			// The gap is shorter than a standard gap
			// (with relative or aboslute tolerance): this
			// is a repeated keypress.
			*repeat_flagp = 1;
		}
		else
		{
			// Standard gap: this is a new keypress.
			*repeat_flagp = 0;
		}
	}
	
	// Calculate extimated time gap remaining for the next code.
	if (is_const(remote)) {
		// The sum (signal_length + gap) is always constant
		// so the gap is shorter when the code is longer.
		if (min_gap(remote) > signal_length) {
			*min_remaining_gapp = min_gap(remote) - signal_length;
			*max_remaining_gapp = max_gap(remote) - signal_length;
		} else {
			*min_remaining_gapp = 0;
			if(max_gap(remote) > signal_length)
			{
				*max_remaining_gapp = max_gap(remote) - signal_length;
			}
			else
			{
				*max_remaining_gapp = 0;
			}
		}
	} else {
		// The gap after the signal is always constant.
		// This is the case of Kanam Accent serial remote.
		*min_remaining_gapp = min_gap(remote);
		*max_remaining_gapp = max_gap(remote);
	}
	
	LOGPRINTF(1, "repeat_flagp:           %d", *repeat_flagp);
	LOGPRINTF(1, "is_const(remote):       %d", is_const(remote));
	LOGPRINTF(1, "remote->gap range:      %lu %lu",
		  (unsigned long) min_gap(remote),
		  (unsigned long) max_gap(remote));
	LOGPRINTF(1, "remote->remaining_gap:  %lu %lu",
		  (unsigned long) remote->min_remaining_gap,
		  (unsigned long) remote->max_remaining_gap);
	LOGPRINTF(1, "signal length:          %lu",
		  (unsigned long) signal_length);
	LOGPRINTF(1, "gap:                    %lu",
		  (unsigned long) gap);
	LOGPRINTF(1, "extim. remaining_gap:   %lu %lu",
		  (unsigned long) *min_remaining_gapp,
		  (unsigned long) *max_remaining_gapp);
	
}

struct ir_ncode *get_code_by_name(struct ir_remote *remote,char *name)
{
	struct ir_ncode *all;

	all=remote->codes;
	while(all->name!=NULL)
	{
		if(strcasecmp(all->name,name)==0)
		{
			return(all);
		}
		all++;
	}
	return(0);
}

struct ir_ncode *get_code(struct ir_remote *remote,
			  ir_code pre,ir_code code,ir_code post,
			  ir_code *toggle_bit_mask_statep)
{
	ir_code pre_mask,code_mask,post_mask,toggle_bit_mask_state,all;
	int found_code, have_code;
	struct ir_ncode *codes,*found;
	
	pre_mask=code_mask=post_mask=0;

	if(has_toggle_bit_mask(remote))
	{
		pre_mask = remote->toggle_bit_mask >>
			   (remote->bits + remote->post_data_bits);
		post_mask = remote->toggle_bit_mask &
		            gen_mask(remote->post_data_bits);
	}
	if(has_ignore_mask(remote))
	{
		pre_mask |= remote->ignore_mask >>
			   (remote->bits + remote->post_data_bits);
		post_mask |= remote->ignore_mask &
		            gen_mask(remote->post_data_bits);
	}
	if(has_toggle_mask(remote) && remote->toggle_mask_state%2)
	{
		ir_code *affected,mask,mask_bit;
		int bit,current_bit;
		
		affected=&post;
		mask=remote->toggle_mask;
		for(bit=current_bit=0;bit<bit_count(remote);bit++,current_bit++)
		{
			if(bit==remote->post_data_bits)
			{
				affected=&code;
				current_bit=0;
			}
			if(bit==remote->post_data_bits+remote->bits)
			{
				affected=&pre;
				current_bit=0;
			}
			mask_bit=mask&1;
			(*affected)^=(mask_bit<<current_bit);
			mask>>=1;
		}
	}
	if(has_pre(remote))
	{
		if((pre|pre_mask)!=(remote->pre_data|pre_mask))
		{
			LOGPRINTF(1,"bad pre data");
#                       ifdef LONG_IR_CODE
			LOGPRINTF(2,"%llx %llx",pre,remote->pre_data);
#                       else
			LOGPRINTF(2,"%lx %lx",pre,remote->pre_data);
#                       endif
			return(0);
		}
		LOGPRINTF(1,"pre");
	}
	
	if(has_post(remote))
	{
		if((post|post_mask)!=(remote->post_data|post_mask))
		{
			LOGPRINTF(1,"bad post data");
#                       ifdef LONG_IR_CODE
			LOGPRINTF(2,"%llx %llx",post,remote->post_data);
#                       else
			LOGPRINTF(2,"%lx %lx",post,remote->post_data);
#                       endif
			return(0);
		}
		LOGPRINTF(1,"post");
	}

	all = gen_ir_code(remote, pre, code, post);

	toggle_bit_mask_state = all&remote->toggle_bit_mask;

	found=NULL;
	found_code=0;
	have_code=0;
	codes=remote->codes;
	if(codes!=NULL)
	{
		while(codes->name!=NULL)
		{
			ir_code next_all;

			next_all = gen_ir_code(remote, remote->pre_data,
					       get_ir_code(codes, codes->current),
					       remote->post_data);
			if(match_ir_code(remote, next_all, all))
			{
				found_code=1;
				if(codes->next!=NULL)
				{
					if(codes->current==NULL)
					{
						codes->current=codes->next;
					}
					else
					{
						codes->current=
							codes->current->next;
					}
				}
				if(!have_code)
				{
					found=codes;
					if(codes->current==NULL)
					{
						have_code=1;
					}
				}
			}
			else
			{
				/* find longest matching sequence */
				struct ir_code_node *search;
				
				search = codes->next;
				if(search == NULL ||
					(codes->next != NULL && codes->current == NULL))
				{
					codes->current=NULL;
				}
				else
				{
					int sequence_match = 0;
					
					while(search != codes->current->next)
					{
						struct ir_code_node *prev, *next;
						int flag = 1;
						
						prev = NULL; /* means codes->code */
						next = search;
						while(next != codes->current)
						{
							if(get_ir_code(codes, prev) != get_ir_code(codes, next))
							{
								flag = 0;
								break;
							}
							prev = get_next_ir_code_node(codes, prev);
							next = get_next_ir_code_node(codes, next);
						}
						if(flag == 1)
						{
							next_all = gen_ir_code(remote, remote->pre_data,
									       get_ir_code(codes, prev),
									       remote->post_data);
							if(match_ir_code(remote, next_all, all))
							{
								codes->current = get_next_ir_code_node(codes, prev);
								sequence_match = 1;
								found_code=1;
								if(!have_code)
								{
									found=codes;
								}
								break;
							}
						}
						search = search->next;
					}
					if(!sequence_match) codes->current = NULL;
				}
			}
			codes++;
		}
	}
#       ifdef DYNCODES
	if(!found_code)
	{
		if(remote->dyncodes[remote->dyncode].code!=code)
		{
			remote->dyncode++;
			remote->dyncode%=2;
		}
		remote->dyncodes[remote->dyncode].code=code;
		found=&(remote->dyncodes[remote->dyncode]);
		found_code=1;
	}
#       endif
	if(found_code && found!=NULL && has_toggle_mask(remote))
	{
		if(!(remote->toggle_mask_state%2))
		{
			remote->toggle_code=found;
			LOGPRINTF(1,"toggle_mask_start");
		}
		else
		{
			if(found!=remote->toggle_code)
			{
				remote->toggle_code=NULL;
				return(NULL);
			}
			remote->toggle_code=NULL;
		}
	}
	*toggle_bit_mask_statep=toggle_bit_mask_state;
	return(found);
}

unsigned long long set_code(struct ir_remote *remote,struct ir_ncode *found,
			    ir_code toggle_bit_mask_state,int repeat_flag,
			    lirc_t min_remaining_gap, lirc_t max_remaining_gap)
{
	unsigned long long code;
	struct timeval current;
	static struct ir_remote *last_decoded = NULL;

	LOGPRINTF(1,"found: %s",found->name);

	gettimeofday(&current,NULL);
	LOGPRINTF(1,"%lx %lx %lx %d %d %d %d %d %d %d",
		  remote,last_remote,last_decoded,
		  remote==last_decoded,
		  found==remote->last_code,
		  found->next!=NULL,
		  found->current!=NULL,
		  repeat_flag,
		  time_elapsed(&remote->last_send,&current)<1000000,
		  (!has_toggle_bit_mask(remote) || toggle_bit_mask_state==remote->toggle_bit_mask_state));
	if(remote->release_detected)
	{
		remote->release_detected = 0;
		if(repeat_flag)
		{
			LOGPRINTF(0, "repeat indicated although release was "
				  "detected before");
		}
		repeat_flag = 0;
	}
	if(remote==last_decoded &&
	   (found==remote->last_code || (found->next!=NULL && found->current!=NULL)) &&
	   repeat_flag &&
	   time_elapsed(&remote->last_send,&current)<1000000 &&
	   (!has_toggle_bit_mask(remote) || toggle_bit_mask_state==remote->toggle_bit_mask_state))
	{
		if(has_toggle_mask(remote))
		{
			remote->toggle_mask_state++;
			if(remote->toggle_mask_state==4)
			{
				remote->reps++;
				remote->toggle_mask_state=2;
			}
		}
		else if(found->current==NULL)
		{
			remote->reps++;
		}
	}
	else
	{
		if(found->next!=NULL && found->current==NULL)
		{
			remote->reps=1;
		}
		else
		{
			remote->reps=0;
		}
		if(has_toggle_mask(remote))
		{
			remote->toggle_mask_state=1;
			remote->toggle_code=found;
		}
		if(has_toggle_bit_mask(remote))
		{
			remote->toggle_bit_mask_state=toggle_bit_mask_state;
		}
	}
	last_remote=remote;
	last_decoded=remote;
	if(found->current==NULL) remote->last_code=found;
	remote->last_send=current;
	remote->min_remaining_gap=min_remaining_gap;
	remote->max_remaining_gap=max_remaining_gap;
	
	code=0;
	if(has_pre(remote))
	{
		code|=remote->pre_data;
		code=code<<remote->bits;
	}
	code|=found->code;
	if(has_post(remote))
	{
		code=code<<remote->post_data_bits;
		code|=remote->post_data;
	}
	if(remote->flags&COMPAT_REVERSE)
	{
		/* actually this is wrong: pre, code and post should
		   be rotated separately but we have to stay
		   compatible with older software
		 */
		code=reverse(code,bit_count(remote));
	}
	return(code);
}

int write_message(char *buffer, size_t size, const char *remote_name,
		  const char *button_name, const char *button_suffix,
		  ir_code code, int reps)
{
	int len;
	
#ifdef __GLIBC__
	/* It seems you can't print 64-bit longs on glibc */
			
	len=snprintf(buffer, size,"%08lx%08lx %02x %s%s %s\n",
		     (unsigned long) (code>>32),
		     (unsigned long) (code&0xFFFFFFFF),
		     reps,
		     button_name, button_suffix,
		     remote_name);
#else
	len=snprintf(buffer, size, "%016llx %02x %s%s %s\n",
		     code,
		     reps,
		     button_name, button_suffix,
		     remote_name);
#endif
	return len;
}

char *decode_all(struct ir_remote *remotes)
{
	struct ir_remote *remote;
	static char message[PACKET_SIZE+1];
	ir_code pre,code,post;
	struct ir_ncode *ncode;
	int repeat_flag;
	ir_code toggle_bit_mask_state;
	lirc_t min_remaining_gap, max_remaining_gap;
	struct ir_remote *scan;
	struct ir_ncode *scan_ncode;
	
	/* use remotes carefully, it may be changed on SIGHUP */
	decoding=remote=remotes;
	while(remote)
	{
		LOGPRINTF(1,"trying \"%s\" remote",remote->name);
		
		if(hw.decode_func(remote,&pre,&code,&post,&repeat_flag,
				  &min_remaining_gap, &max_remaining_gap) &&
		   (ncode=get_code(remote,pre,code,post,&toggle_bit_mask_state)))
		{
			int len;
			int reps;

			code=set_code(remote,ncode,toggle_bit_mask_state,
				      repeat_flag,
				      min_remaining_gap,
				      max_remaining_gap);
			if((has_toggle_mask(remote) &&
			    remote->toggle_mask_state%2) ||
			   ncode->current!=NULL)
			{
				decoding=NULL;
				return(NULL);
			}

			for(scan = decoding; scan != NULL; scan = scan->next)
			{
				for( scan_ncode = scan->codes; scan_ncode->name != NULL; scan_ncode++)
				{
					scan_ncode->current = NULL;
				}
			}
			if(is_xmp(remote))
			{
				remote->last_code->current = remote->last_code->next;
			}
			reps =  remote->reps-(ncode->next ? 1:0);
			if(reps > 0)
			{
				if(reps <= remote->suppress_repeat)
				{
					decoding=NULL;
					return NULL;
				}
				else
				{
					reps -= remote->suppress_repeat;
				}
			}
			register_button_press
				(remote, remote->last_code,
				 code, reps);
			
			len = write_message(message, PACKET_SIZE+1,
					    remote->name,
					    remote->last_code->name, "", code,
					    reps);
			decoding=NULL;
			if(len>=PACKET_SIZE+1)
			{
				logprintf(LOG_ERR,"message buffer overflow");
				return(NULL);
			}
			else
			{
				return(message);
			}
		}
		else
		{
			LOGPRINTF(1,"failed \"%s\" remote",remote->name);
		}
		remote->toggle_mask_state=0;
		remote=remote->next;
	}
	decoding=NULL;
	last_remote=NULL;
	LOGPRINTF(1,"decoding failed for all remotes");
	return(NULL);
}

int send_ir_ncode(struct ir_remote *remote,struct ir_ncode *code)
{
	int ret;

#if !defined(SIM_SEND)
	/* insert pause when needed: */
	if(remote->last_code!=NULL)
	{
		struct timeval current;
		unsigned long usecs;
		
		gettimeofday(&current,NULL);
		usecs = time_left(&current, &remote->last_send,
				  remote->min_remaining_gap*2);
		if(usecs>0) 
		{
			if(repeat_remote==NULL ||
			   remote!=repeat_remote ||
			   remote->last_code!=code)
			{
				usleep(usecs);
			}
		}
	}
#endif

	ret = hw.send_func(remote, code);
	
	if(ret)
	{
		gettimeofday(&remote->last_send, NULL);
		remote->last_code = code;
	}
	
	return ret;
 }
