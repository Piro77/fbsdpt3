/*******************************************************************************
   earthsoft PT3 Linux driver

   This program is free software; you can redistribute it and/or modify it
   under the terms and conditions of the GNU General Public License,
   version 3, as published by the Free Software Foundation.

   This program is distributed in the hope it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   The full GNU General Public License is included in this distribution in
   the file called "COPYING".

 *******************************************************************************/

#ifndef _H_PT3_MX
#define _H_PT3_MX

#include "pt3_bus.h"
#include "pt3_i2c.h"
#include "pt3_tc.h"

typedef struct _PT3_MX {
	PT3_I2C *i2c;
	PT3_TC *tc;
	__u32 freq;
	int sleep;
	__u32 channel;
	__s32 offset;
} PT3_MX;

__u8 pt3_mx_address(__u32 index);
void pt3_mx_get_channel_frequency(PT3_MX *mx, __u32 channel, int *catv, __u32 *number, __u32 *freq);
STATUS pt3_mx_set_frequency(PT3_MX *mx, __u32 channel, __s32 offset);
STATUS pt3_mx_set_sleep(PT3_MX *mx, int sleep);
PT3_MX * create_pt3_mx(PT3_I2C *i2c, PT3_TC *tc);
void free_pt3_mx(PT3_MX *mx);

STATUS pt3_mx_get_locked1(PT3_MX *mx, PT3_BUS *bus, int *locked);
STATUS pt3_mx_get_locked2(PT3_MX *mx, PT3_BUS *bus, int *locked);

#endif
