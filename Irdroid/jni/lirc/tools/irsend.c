/* 	$Id: irsend.c,v 5.6 2010/08/27 05:08:09 jarodwilson Exp $	 */

/*
  
  irsend -  application for sending IR-codes via lirc
  
  Copyright (C) 1998 Christoph Bartelmus (lirc@bartelmus.de)
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#define PACKET_SIZE 256
/* three seconds */
#define TIMEOUT 3

int timeout=0;
char *progname;

//void sigalrm(int sig)
//{
//	timeout=1;
//}

const char *read_string(int fd)
{
	static char buffer[PACKET_SIZE+1]="";
	char *end;
	static int ptr=0;
	ssize_t ret;
		
	if(ptr>0)
	{
		memmove(buffer,buffer+ptr,strlen(buffer+ptr)+1);
		ptr=strlen(buffer);
		end=strchr(buffer,'\n');
	}
	else
	{
		end=NULL;
	}
	alarm(TIMEOUT);
	while(end==NULL)
	{
		if(PACKET_SIZE<=ptr)
		{
			fprintf(stderr,"%s: bad packet\n",progname);
			ptr=0;
			return(NULL);
		}
		ret=read(fd,buffer+ptr,PACKET_SIZE-ptr);

		if(ret<=0 || timeout)
		{
			if(timeout)
			{
				fprintf(stderr,"%s: timeout\n",progname);
			}
			else
			{
				alarm(0);
			}
			ptr=0;
			return(NULL);
		}
		buffer[ptr+ret]=0;
		ptr=strlen(buffer);
		end=strchr(buffer,'\n');
	}
	alarm(0);timeout=0;

	end[0]=0;
	ptr=strlen(buffer)+1;
#       ifdef DEBUG
	printf("buffer: -%s-\n",buffer);
#       endif
	return(buffer);
}

enum packet_state
{
	P_BEGIN,
	P_MESSAGE,
	P_STATUS,
	P_DATA,
	P_N,
	P_DATA_N,
	P_END
};

int send_packet(int fd,const char *packet)
{
	int done,todo;
	const char *string,*data;
	char *endptr;
	enum packet_state state;
	int status,n;
	unsigned long data_n=0;

	todo=strlen(packet);
	data=packet;
	while(todo>0)
	{
		done=write(fd,(void *) data,todo);
		if(done<0)
		{
			fprintf(stderr,"%s: could not send packet\n",
				progname);
			perror(progname);
			return(-1);
		}
		data+=done;
		todo-=done;
	}

	/* get response */
	status=0;
	state=P_BEGIN;
	n=0;
	while(1)
	{
		string=read_string(fd);
		if(string==NULL) return(-1);
		switch(state)
		{
		case P_BEGIN:
			if(strcasecmp(string,"BEGIN")!=0)
			{
				continue;
			}
			state=P_MESSAGE;
			break;
		case P_MESSAGE:
			if(strncasecmp(string,packet,strlen(string))!=0 ||
			   strlen(string)+1!=strlen(packet))
			{
				state=P_BEGIN;
				continue;
			}
			state=P_STATUS;
			break;
		case P_STATUS:
			if(strcasecmp(string,"SUCCESS")==0)
			{
				status=0;
			}
			else if(strcasecmp(string,"END")==0)
			{
				status=0;
				return(status);
			}
			else if(strcasecmp(string,"ERROR")==0)
			{
				fprintf(stderr,"%s: command failed: %s",
					progname,packet);
				status=-1;
			}
			else
			{
				goto bad_packet;
			}
			state=P_DATA;
			break;
		case P_DATA:
			if(strcasecmp(string,"END")==0)
			{
				return(status);
			}
			else if(strcasecmp(string,"DATA")==0)
			{
				state=P_N;
				break;
			}
			goto bad_packet;
		case P_N:
			errno=0;
			data_n=strtoul(string,&endptr,0);
			if(!*string || *endptr)
			{
				goto bad_packet;
			}
			if(data_n==0)
			{
				state=P_END;
			}
			else
			{
				state=P_DATA_N;
			}
			break;
		case P_DATA_N:
			fprintf(stderr,"%s: %s\n",progname,string);
			n++;
			if(n==data_n) state=P_END;
			break;
		case P_END:
			if(strcasecmp(string,"END")==0)
			{
				return(status);
			}
			goto bad_packet;
			break;
		}
	}
 bad_packet:
	fprintf(stderr,"%s: bad return packet\n",progname);
	return(-1);
}

int sendIrCode(int argc,char **argv)
{
	ZLOG_INFO(argv[0]);
	ZLOG_INFO(argv[1]);
	ZLOG_INFO(argv[2]);
	ZLOG_INFO(argv[3]);

	char *directive;
	char *remote;
	char *code;
    char *lircd=NULL;
	char *address=NULL;
	unsigned short port = LIRC_INET_PORT;
	unsigned long count=1;
	struct sockaddr_un addr_un;

	int fd;
	char buffer[PACKET_SIZE+1];
	char * err_mesg;
//	struct sigaction act;
	
	progname = "irsend";

	lircd=LIRCD;
	
//	act.sa_handler=sigalrm;
//	sigemptyset(&act.sa_mask);
//	act.sa_flags=0;           /* we need EINTR */
//	sigaction(SIGALRM,&act,NULL);

	addr_un.sun_family=AF_UNIX;
	strcpy(addr_un.sun_path,lircd);
	fd=socket(AF_UNIX,SOCK_STREAM,0);

	if(fd==-1)
	{
		sprintf(err_mesg,"%s: could not open socket\n",progname);
		ZLOG_INFO(err_mesg);
		perror(progname);
		return 0;
	};
	ZLOG_INFO(lircd);
	if(connect(fd, (struct sockaddr *) &addr_un, sizeof(addr_un)) == -1)
	{
		sprintf(err_mesg,"%s: could not connect to socket\n",progname);
		ZLOG_INFO(err_mesg);
		perror(progname);
		return 0;
	};
	ZLOG_INFO("irsend: Connected to socket");
	
/*	if(address) free(address);
	address = NULL;
	
	directive=argv[optind++];

	remote=argv[optind++];

	if (optind==argc)
	{
		sprintf(err_mesg,"%s: not enough arguments\n",progname);
		ZLOG_INFO(err_mesg);
		return 0;
	}
	while(optind<argc)
	{
		code=argv[optind++];

		if(strlen(directive)+strlen(remote)+strlen(code)+3<PACKET_SIZE)
		{
			if(strcasecmp(directive,"SEND_ONCE")==0 && count>1)
			{
				sprintf(buffer,"%s %s %s %lu\n",
					directive,remote,code,count);
			}
			else
			{
				sprintf(buffer,"%s %s %s\n",directive,remote,code);
			}
			if(send_packet(fd,buffer)==-1)
			{
				return 0;
			}
		}
		else
		{
			sprintf(err_mesg,"%s: input too long\n",progname);
			ZLOG_INFO(err_mesg);
			return 0;
		}
	}*/
	close(fd);
	return 1;
}

