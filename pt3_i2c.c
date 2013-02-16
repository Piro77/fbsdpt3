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

#if defined(__FreeBSD__)
#include "pt3_misc.h"
#else
#include "version.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
#include <asm/system.h>
#endif
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#endif

#include "pt3_com.h"
#include "pt3_pci.h"
#include "pt3_i2c.h"

#define DATA_OFFSET 2048

static void
wait(PT3_I2C *i2c, __u32 *data)
{
	__u32 val;
	
	while (1) {
#if defined(__FreeBSD__)
		struct ptx_softc *scp;
		scp = i2c->scp;
		val = bus_space_read_4(scp->bt, scp->bh, REGS_I2C_R);
#else
		val = readl(i2c->bar[0] + REGS_I2C_R);
#endif
		if (!BIT_SHIFT_MASK(val, 0, 1))
			break;
		schedule_timeout_interruptible(msecs_to_jiffies(1));	
	}

	if (data != NULL)
		*data = val;
}

static STATUS
run_code(PT3_I2C *i2c, __u32 start_addr, __u32 *ack)
{
	__u32 data, a;

	wait(i2c, &data);

	if (unlikely(start_addr >= (1 << 13)))
		PT3_PRINTK(0, KERN_DEBUG, "start address is over.\n");
	
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = i2c->scp;
	bus_space_write_4(scp->bt,scp->bh, REGS_I2C_W, 1 << 16 | start_addr);
#else
	writel(1 << 16 | start_addr, i2c->bar[0] + REGS_I2C_W);
#endif
#if 0
	PT3_PRINTK(7, KERN_DEBUG, "run i2c start_addr=0x%x\n", start_addr);
#endif

	wait(i2c, &data);

	a = BIT_SHIFT_MASK(data, 1, 2);
	if (ack != NULL)
		*ack = a;
	if (a)
		PT3_PRINTK(0, KERN_DEBUG, "fail i2c run_code status 0x%x\n", data);

	return BIT_SHIFT_MASK(data, 1, 2) ? STATUS_I2C_ERROR : STATUS_OK;
}

void
pt3_i2c_copy(PT3_I2C *i2c, PT3_BUS *bus)
{
#if defined(__FreeBSD__)
	uint8_t	dst;
#else
	void __iomem *dst;
#endif
	__u8 *src;
	__u32 i;

	src = &bus->insts[0];
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = i2c->scp;
	dst = DATA_OFFSET + (bus->inst_addr / 2);
#else
	dst = i2c->bar[1] + DATA_OFFSET + (bus->inst_addr / 2);
#endif

#if 0
	PT3_PRINTK(7, KERN_DEBUG, "PT3 : i2c_copy. base=%p dst=%p src=%p size=%d\n",
						i2c->bar[1], dst, src, bus->inst_pos);
#endif

#if 1
	for (i = 0; i < bus->inst_pos; i++) {
#if defined(__FreeBSD__)
		bus_space_write_1(scp->pt3_bt,scp->pt3_bh, dst + i , src[i]);
#else
		writeb(src[i], dst + i);
#endif
	}
#else
	memcpy(dst, src, bus->inst_pos);
#endif
}

STATUS
pt3_i2c_run(PT3_I2C *i2c, PT3_BUS *bus, __u32 *ack, int copy)
{
	STATUS status;
	__u32 rsize, i;

#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = i2c->scp;
	mtx_lock(&scp->lock);
	while(scp->i2c_progress) {
		msleep(&scp->i2c_progress, &scp->lock, 0|PCATCH, "ptxi2w", 0);
	}
	scp->i2c_progress = 1;
	mtx_unlock(&scp->lock);
#else
	mutex_lock(&i2c->lock);
#endif

	if (copy) {
		pt3_i2c_copy(i2c, bus);
	}

	status = run_code(i2c, bus->inst_addr, ack);

	rsize = bus->read_addr;

	for (i = 0; i < rsize; i++) {
#if defined(__FreeBSD__)
		pt3_bus_push_read_data(bus, bus_space_read_1(scp->pt3_bt,scp->pt3_bh, DATA_OFFSET +
i));
#else
		pt3_bus_push_read_data(bus, readb(i2c->bar[1] + DATA_OFFSET + i));
#endif
	}
#if 0
	if (rsize > 0) {
		for (i = 1; i < 10; i++) {
			PT3_PRINTK(7, KERN_DEBUG, "bus_read_data + %d = 0x%x inst = 0x%x\n",
					i, readb(i2c->bar[1] + DATA_OFFSET + i),
					bus->insts[i]);
		}
	}
#endif

#if defined(__FreeBSD__)
	mtx_lock(&scp->lock);
	scp->i2c_progress = 0;
	wakeup(&scp->i2c_progress);
	mtx_unlock(&scp->lock);
#else
	mutex_unlock(&i2c->lock);
#endif

	return status;
}

int
pt3_i2c_is_clean(PT3_I2C *i2c)
{
	__u32 val;

#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = i2c->scp;
	val = bus_space_read_4(scp->bt,scp->bh,REGS_I2C_R);
#else
	val = readl(i2c->bar[0] + REGS_I2C_R);
#endif

	return BIT_SHIFT_MASK(val, 3, 1);
}

void
pt3_i2c_reset(PT3_I2C *i2c)
{
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = i2c->scp;
	bus_space_write_4(scp->bt,scp->bh, REGS_I2C_W , 1<<17);
#else
	writel(1 << 17, i2c->bar[0] + REGS_I2C_W);
#endif
}

#if defined(__FreeBSD__)
PT3_I2C *
create_pt3_i2c(void *p)
{
	PT3_I2C *i2c;
	struct ptx_softc *scp;
	scp = (struct ptx_softc *)p;
	
	i2c = pt3_vzalloc(sizeof(PT3_I2C));
	if (i2c == NULL)
		return NULL;
	i2c->scp = scp;

	return i2c;
}
void
free_pt3_i2c(PT3_I2C *i2c)
{
	vfree(i2c);
}
#else
PT3_I2C *
create_pt3_i2c(__u8 __iomem *bar[])
{
	PT3_I2C *i2c;

	i2c = pt3_vzalloc(sizeof(PT3_I2C));
	if (i2c == NULL)
		goto fail;

	mutex_init(&i2c->lock);
	i2c->bar[0] = bar[0];
	i2c->bar[1] = bar[1];

	return i2c;
fail:
	if (i2c != NULL)
		free_pt3_i2c(i2c);
	return NULL;
}

void
free_pt3_i2c(PT3_I2C *i2c)
{
	vfree(i2c);
}
#endif
