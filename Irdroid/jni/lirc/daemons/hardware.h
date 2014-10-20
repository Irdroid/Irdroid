/*      $Id: hardware.h,v 5.13 2010/04/11 18:50:38 lirc Exp $      */

/****************************************************************************
 ** hardware.h **************************************************************
 ****************************************************************************
 *
 * hardware.h - internal hardware interface
 *
 * Copyright (C) 1999 Christoph Bartelmus <lirc@bartelmus.de>
 *
 */ 

#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "drivers/lirc.h"
#include "ir_remote_types.h"

struct hardware
{
	char *device;
	int fd;
	unsigned long features;
	unsigned long send_mode;
	unsigned long rec_mode;
	unsigned long code_length;
	int (*init_func)(void);
	int (*deinit_func)(void);
	int (*send_func)(struct ir_remote *remote,struct ir_ncode *code);
	char *(*rec_func)(struct ir_remote *remotes);
	int (*decode_func)(struct ir_remote *remote,
			   ir_code *prep,ir_code *codep,ir_code *postp,
			   int *repeat_flag,
			   lirc_t *min_remaining_gapp,
			   lirc_t *max_remaining_gapp);
	int (*ioctl_func)(unsigned int cmd, void *arg);
	lirc_t (*readdata)(lirc_t timeout);
	char *name;
	
	unsigned int resolution;
};

extern struct hardware hw;
#endif
