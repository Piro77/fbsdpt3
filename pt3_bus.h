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

#ifndef		__PT3_BUS_H__
#define		__PT3_BUS_H__

#include <linux/mutex.h>
#include "pt3_pci.h"

#define PT3_BUS_MAX_INST 4096
#define PT3_BUS_INST_ADDR0 4096 + 0
#define PT3_BUS_INST_ADDR1 4096 + 2042

typedef struct _PT3_BUS {
	__u32 inst_addr;
	__u32 inst_count;
	__u32 read_addr;
	__u8 tmp_inst;
	__u8 insts[PT3_BUS_MAX_INST];
	__u32 inst_pos;
	__u8 *buf;
	__u32 buf_pos;
	__u32 buf_size;
} PT3_BUS;

void pt3_bus_start(PT3_BUS *bus);
void pt3_bus_stop(PT3_BUS *bus);
void pt3_bus_write(PT3_BUS *bus, const __u8 *data, __u32 size);
__u32 pt3_bus_read(PT3_BUS *bus, __u8 *data, __u32 size);
void pt3_bus_reset(PT3_BUS *bus);
void pt3_bus_sleep(PT3_BUS *bus, __u32 ms);
void pt3_bus_end(PT3_BUS *bus);
__u8 pt3_bus_data1(PT3_BUS *bus, __u32 index);
void pt3_bus_push_read_data(PT3_BUS *bus, __u8 data);

PT3_BUS * create_pt3_bus(void);
void free_pt3_bus(PT3_BUS *bus);

#endif
