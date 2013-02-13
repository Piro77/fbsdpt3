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

uint8_t get_base_addr(PT3_DMA *dma);

static uint8_t *
alloc_dmabuf(struct ptx_softc *scp, PT3_DMA_PAGE *page, bus_dma_tag_t dmat);
static void
set_addr(void *arg, bus_dma_segment_t *segs, int nseg, int error);
static void
free_dmabuf(PT3_DMA_PAGE *page, bus_dma_tag_t dmat);

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

#include "pt3_com.h"
#include "pt3_pci.h"
#include "pt3_i2c.h"
#include "pt3_bus.h"
#include "pt3_dma.h"

#endif

#define DMA_DESC_SIZE		20
#define DMA_PAGE_SIZE		4096
#define MAX_DESCS			204		/* 4096 / 20 */
#if 1
#define BLOCK_COUNT			(17)
#define BLOCK_SIZE			(DMA_PAGE_SIZE * 47)
#else
#define BLOCK_COUNT			(32)
#define BLOCK_SIZE			(DMA_PAGE_SIZE * 47 * 8)
#endif
#define DMA_TS_BUF_SIZE		(BLOCK_SIZE * BLOCK_COUNT)
#define NOT_SYNC_BYTE		0x74

static __u32
gray2binary(__u32 gray, __u32 bit)
{
	__u32 binary, i, j, k;

	binary = 0;
	for (i = 0; i < bit; i++) {
		k = 0;
		for (j = i; j < bit; j++) {
			k = k ^ BIT_SHIFT_MASK(gray, j, 1);
		}
		binary |= k << i;
	}

	return binary;
}

static void
dma_link_descriptor(__u64 next_addr, __u8 *desc)
{
	(*(__u64 *)(desc + 12)) = next_addr | 2;
}

static void
dma_write_descriptor(__u64 ts_addr, __u32 size, __u64 next_addr, __u8 *desc)
{
	(*(__u64 *)(desc +  0)) = ts_addr   | 7;
	(*(__u32 *)(desc +  8)) = size      | 7;
	(*(__u64 *)(desc + 12)) = next_addr | 2;
}

void
pt3_dma_build_page_descriptor(PT3_DMA *dma, int loop)
{
	PT3_DMA_PAGE *desc_info, *ts_info;
	__u64 ts_addr, desc_addr;
	__u32 i, j, ts_size, desc_remain, ts_info_pos, desc_info_pos;
	__u8 *prev, *curr;

	if (unlikely(dma == NULL)) {
		PT3_PRINTK(1, KERN_ERR, "dma build page descriptor needs DMA\n");
		return;
	}
#if 0
	PT3_PRINTK(7, KERN_DEBUG, "build page descriptor ts_count=%d ts_size=0x%x desc_count=%d desc_size=0x%x\n",
			dma->ts_count, dma->ts_info[0].size, dma->desc_count, dma->desc_info[0].size);
#endif

	desc_info_pos = ts_info_pos = 0;
	desc_info = &dma->desc_info[desc_info_pos];
#if 1
	if (unlikely(desc_info == NULL)) {
		PT3_PRINTK(0, KERN_ERR, "dma maybe failed allocate desc_info %d\n",
				desc_info_pos);
		return;
	}
#endif
	desc_addr = desc_info->addr;
	desc_remain = desc_info->size;
	desc_info->data_pos = 0;
	curr = &desc_info->data[desc_info->data_pos];
	prev = NULL;
#if 1
	if (unlikely(curr == NULL)) {
		PT3_PRINTK(0, KERN_ERR, "dma maybe failed allocate desc_info->data %d\n",
				desc_info_pos);
		return;
	}
#endif
	desc_info_pos++;

	for (i = 0; i < dma->ts_count; i++) {
#if 1
		if (unlikely(dma->ts_count <= ts_info_pos)) {
			PT3_PRINTK(0, KERN_ERR, "ts_info overflow max=%d curr=%d\n",
					dma->ts_count, ts_info_pos);
			return;
		}
#endif
		ts_info = &dma->ts_info[ts_info_pos];
#if 1
		if (unlikely(ts_info == NULL)) {
			PT3_PRINTK(0, KERN_ERR, "dma maybe failed allocate ts_info %d\n",
					ts_info_pos);
			return;
		}
#endif
		ts_addr = ts_info->addr;
		ts_size = ts_info->size;
		ts_info_pos++;
		// PT3_PRINTK(7, KERN_DEBUG, "ts_info addr=0x%llx size=0x%x\n", ts_addr, ts_size);
#if 1
		if (unlikely(ts_info == NULL)) {
			PT3_PRINTK(0, KERN_ERR, "dma maybe failed allocate ts_info %d\n",
					ts_info_pos);
			return;
		}
#endif
		for (j = 0; j < ts_size / DMA_PAGE_SIZE; j++) {
			if (desc_remain < DMA_DESC_SIZE) {
#if 1
				if (unlikely(dma->desc_count <= desc_info_pos)) {
					PT3_PRINTK(0, KERN_ERR, "desc_info overflow max=%d curr=%d\n",
							dma->desc_count, desc_info_pos);
					return;
				}
#endif
				desc_info = &dma->desc_info[desc_info_pos];
				desc_info->data_pos = 0;
				curr = &desc_info->data[desc_info->data_pos];
#if 1
				if (unlikely(curr == NULL)) {
					PT3_PRINTK(0, KERN_ERR, "dma maybe failed allocate desc_info->data %d\n",
							desc_info_pos);
					return;
				}
				/*
				PT3_PRINTK(7, KERN_DEBUG, "desc_info_pos=%d ts_addr=0x%llx remain=%d\n",
						desc_info_pos, ts_addr, desc_remain);
				*/
#endif
				desc_addr = desc_info->addr;
				desc_remain = desc_info->size;
				desc_info_pos++;
			}
			if (prev != NULL) {
				dma_link_descriptor(desc_addr, prev);
			}
			dma_write_descriptor(ts_addr, DMA_PAGE_SIZE, 0, curr);
#if 0
			PT3_PRINTK(7, KERN_DEBUG, "dma write desc ts_addr=0x%llx desc_info_pos=%d\n",
						ts_addr, desc_info_pos);
#endif
			ts_addr += DMA_PAGE_SIZE;

			prev = curr;
			desc_info->data_pos += DMA_DESC_SIZE;
			if (unlikely(desc_info->size <= desc_info->data_pos)) {
				PT3_PRINTK(0, KERN_ERR, "dma desc_info data overflow.\n");
				return;
			}
			curr = &desc_info->data[desc_info->data_pos];
			desc_addr += DMA_DESC_SIZE;
			desc_remain -= DMA_DESC_SIZE;
		}
	}

	if (prev != NULL) {
		if (loop)
			dma_link_descriptor(dma->desc_info->addr, prev);
		else
			dma_link_descriptor(1, prev);
	}
}

#if defined(__FreeBSD__)
uint8_t
#else
void __iomem *
#endif
get_base_addr(PT3_DMA *dma)
{
#if defined(__FreeBSD__)
	return REGS_DMA_DESC_L + (0x18 * dma->real_index);
#else
	return scp->bar[0] + REGS_DMA_DESC_L + (0x18 * dma->real_index);
#endif
}

void
pt3_dma_set_test_mode(PT3_DMA *dma, int test, __u16 init, int not, int reset)
{
#if defined(__FreeBSD__)
	uint8_t base;
#else
	void __iomem *base;
#endif
	__u32 data;

	base = get_base_addr(dma);
	data = (reset ? 1: 0) << 18 | (not ? 1 : 0) << 17 | (test ? 1 : 0) << 16 | init;

	PT3_PRINTK(7, KERN_DEBUG, "set_test_mode base=%x data=0x%04d\n",
			base, data);

#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = dma->i2c->scp;
	bus_space_write_4(scp->bt, scp->bh, base + 0x0c, data);
#else
	writel(data, base + 0x0c);
#endif

}

void
pt3_dma_set_enabled(PT3_DMA *dma, int enabled)
{
#if defined(__FreeBSD__)
	uint8_t base;
#else
	void __iomem *base;
#endif
	__u32 data;
	__u64 start_addr;
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = dma->i2c->scp;
#endif

	base = get_base_addr(dma);
	start_addr = dma->desc_info->addr;

	if (enabled) {
		PT3_PRINTK(7, KERN_DEBUG, "enable dma real_index=%d start_addr=%llx\n",
				dma->real_index, start_addr);
		pt3_dma_reset(dma);
#if defined(__FreeBSD__)
		bus_space_write_4(scp->bt,scp->bh,base + 0x08, 1 << 1);
		bus_space_write_4(scp->bt,scp->bh,base + 0x0, BIT_SHIFT_MASK(start_addr,  0, 32));
		bus_space_write_4(scp->bt,scp->bh,base + 0x4, BIT_SHIFT_MASK(start_addr,  32, 32));
#else
		writel( 1 << 1, base + 0x08);
		writel(BIT_SHIFT_MASK(start_addr,  0, 32), base + 0x0);
		writel(BIT_SHIFT_MASK(start_addr, 32, 32), base + 0x4);
#endif
		PT3_PRINTK(7, KERN_DEBUG, "set descriptor address low %llx\n",
				BIT_SHIFT_MASK(start_addr,  0, 32));
		PT3_PRINTK(7, KERN_DEBUG, "set descriptor address heigh %llx\n",
				BIT_SHIFT_MASK(start_addr, 32, 32));
#if defined(__FreeBSD__)
		bus_space_write_4(scp->bt, scp->bh, base + 0x08, 1 << 0);
#else
		writel( 1 << 0, base + 0x08);
#endif

	} else {
		PT3_PRINTK(7, KERN_DEBUG, "disable dma real_index=%d\n", dma->real_index);
#if defined(__FreeBSD__)
		bus_space_write_4(scp->bt,scp->bh,base + 0x08 , 1 << 1);
#else
		writel(1 << 1, base + 0x08);
#endif

		while (1) {
#if defined(__FreeBSD__)
			data = bus_space_read_4(scp->bt, scp->bh, base + 0x10);
#else
			data = readl(base + 0x10);
#endif

			if (!BIT_SHIFT_MASK(data, 0, 1))
				break;
			schedule_timeout_interruptible(msecs_to_jiffies(1));
		}
	}
	dma->enabled = enabled;
}

ssize_t
#if defined(__FreeBSD__)
pt3_dma_copy(PT3_DMA *dma, struct uio *uio, size_t size, int look_ready)
#else
pt3_dma_copy(PT3_DMA *dma, char __user *buf, size_t size, loff_t *ppos, int look_ready)
#endif
{
	int ready;
	PT3_DMA_PAGE *page;
	size_t csize, remain;
	__u32 lp;
	__u32 prev;

#if defined(__FreeBSD__)
	mtx_lock(&dma->lock);
#else
	mutex_lock(&dma->lock);
#endif

	PT3_PRINTK(7, KERN_DEBUG, "dma_copy ts_pos=0x%x data_pos=0x%x size %d\n",
				dma->ts_pos, dma->ts_info[dma->ts_pos].data_pos,size);

	remain = size;
	for (;;) {
		if (likely(look_ready)) {
			for (lp = 0; lp < 20; lp++) {
				ready = pt3_dma_ready(dma);
				if (ready)
					break;
				schedule_timeout_interruptible(msecs_to_jiffies(30));
			}
			if (!ready)
				goto last;
			prev = dma->ts_pos - 1;
			if (prev < 0 || dma->ts_count <= prev)
				prev = dma->ts_count - 1;
			if (dma->ts_info[prev].data[0] != NOT_SYNC_BYTE)
				PT3_PRINTK(7, KERN_INFO, "dma buffer overflow. prev=%d data=0x%x\n",
						prev, dma->ts_info[prev].data[0]);
		}
		page = &dma->ts_info[dma->ts_pos];
		for (;;) {
			if ((page->size - page->data_pos) > remain) {
				csize = remain;
			} else {
				csize = (page->size - page->data_pos);
			}
#if defined(__FreeBSD__)
			int rv;
			rv = uiomove(&page->data[page->data_pos],csize,uio);
			if (rv) {
				mtx_unlock(&dma->lock);
				PT3_PRINTK(7, KERN_DEBUG, "uiomove error %d\n",rv);
				return rv;
			}
#else
			if (copy_to_user(&buf[size - remain], &page->data[page->data_pos], csize)) {
				mutex_unlock(&dma->lock);
				return -EFAULT;
			}
			*ppos += csize;
#endif
			remain -= csize;
			page->data_pos += csize;
			if (page->data_pos >= page->size) {
				page->data_pos = 0;
				page->data[page->data_pos] = NOT_SYNC_BYTE;
				dma->ts_pos++;
				if (dma->ts_pos >= dma->ts_count)
					dma->ts_pos = 0;
				break;
			}
			if (remain <= 0)
				goto last;
		}
		// schedule_timeout_interruptible(msecs_to_jiffies(0));
	}
last:
#if defined(__FreeBSD__)
	mtx_unlock(&dma->lock);
#else
	mutex_unlock(&dma->lock);
#endif

	return size - remain;
}

int
pt3_dma_ready(PT3_DMA *dma)
{
	__u32 next;
	PT3_DMA_PAGE *page;
	__u8 *p;

	next = dma->ts_pos + 1;
	if (next >= dma->ts_count)
		next = 0;

	page = &dma->ts_info[next];
	p = &page->data[page->data_pos];

	if (*p == 0x47)
		return 1;
	if (*p == NOT_SYNC_BYTE)
		return 0;

	PT3_PRINTK(0, KERN_DEBUG, "invalid sync byte value=0x%02x ts_pos=%d data_pos=%d curr=0x%02x\n",
			*p, next, page->data_pos, dma->ts_info[dma->ts_pos].data[0]);

	return 0;
}

void
pt3_dma_reset(PT3_DMA *dma)
{
	PT3_DMA_PAGE *page;
	__u32 i;

	for (i = 0; i < dma->ts_count; i++) {
		page = &dma->ts_info[i];
		memset(page->data, 0, page->size);
		page->data_pos = 0;
		*page->data = NOT_SYNC_BYTE;
	}
	dma->ts_pos = 0;
}

__u32
pt3_dma_get_ts_error_packet_count(PT3_DMA *dma)
{
#if defined(__FreeBSD__)
	uint8_t base;
#else
	void __iomem *base;
#endif
	__u32 gray;
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = dma->i2c->scp;
#endif

	base = get_base_addr(dma);

#if defined(__FreeBSD__)
	gray = bus_space_read_4(scp->bt, scp->bh, base + 0x14 );
#else
	gray = readl(base + 0x14);
#endif


	return gray2binary(gray, 32);
}

__u32
pt3_dma_get_status(PT3_DMA *dma)
{
#if defined(__FreeBSD__)
	uint8_t base;
#else
	void __iomem *base;
#endif
	__u32 status;
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = dma->i2c->scp;
#endif

	base = get_base_addr(dma);

#if defined(__FreeBSD__)
	status = bus_space_read_4(scp->bt, scp->bh, base + 0x10 );
#else
	status = readl(base + 0x10);
#endif


	return status;
}

PT3_DMA *
#if defined(__FreeBSD__)
create_pt3_dma(void *p, PT3_I2C *i2c, int real_index)
#else
reate_pt3_dma(struct pci_dev *hwdev, PT3_I2C *i2c, int real_index)
#endif
{
	PT3_DMA *dma;
	PT3_DMA_PAGE *page;
	__u32 i;
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = p;
#endif

	dma = kzalloc(sizeof(PT3_DMA), GFP_KERNEL);
	if (dma == NULL) {
		PT3_PRINTK(0, KERN_ERR, "fail allocate PT3_DMA\n");
		goto fail;
	}

	dma->enabled = 0;
	dma->i2c = i2c;
	dma->real_index = real_index;
#if defined(__FreeBSD__)
	mtx_init(&dma->lock, "pt3dma", NULL, MTX_DEF);
#else
	mutex_init(&dma->lock);
#endif
	
	dma->ts_count = BLOCK_COUNT;
	dma->ts_info = kzalloc(sizeof(PT3_DMA_PAGE) * dma->ts_count, GFP_KERNEL);
	if (dma->ts_info == NULL) {
		PT3_PRINTK(0, KERN_ERR, "fail allocate PT3_DMA_PAGE\n");
		goto fail;
	}
	for (i = 0; i < dma->ts_count; i++) {
		page = &dma->ts_info[i];
		page->size = BLOCK_SIZE;
		page->data_pos = 0;
#if defined(__FreeBSD__)
                page->data = alloc_dmabuf(scp, page, scp->pt3_dmat);
#else
		page->data = pci_alloc_consistent(hwdev, page->size, &page->addr);
#endif
		if (page->data == NULL) {
			PT3_PRINTK(0, KERN_ERR, "fail allocate consistent. %d\n", i);
			goto fail;
		}
	}
	PT3_PRINTK(7, KERN_DEBUG, "Allocate TS buffer.\n");

	dma->desc_count = (DMA_TS_BUF_SIZE / (DMA_PAGE_SIZE) + MAX_DESCS - 1) / MAX_DESCS;
	dma->desc_info = kzalloc(sizeof(PT3_DMA_PAGE) * dma->desc_count, GFP_KERNEL);
	if (dma->desc_info == NULL) {
		PT3_PRINTK(0, KERN_ERR, "fail allocate PT3_DMA_PAGE\n");
		goto fail;
	}
	for (i = 0; i < dma->desc_count; i++) {
		page = &dma->desc_info[i];
		page->size = DMA_PAGE_SIZE;
		page->data_pos = 0;
#if defined(__FreeBSD__)
                page->data = alloc_dmabuf(scp, page, scp->dmat);
#else
		page->data = pci_alloc_consistent(hwdev, page->size, &page->addr);
#endif
		if (page->data == NULL) {
			PT3_PRINTK(0, KERN_ERR, "fail allocate consistent. %d\n", i);
			goto fail;
		}
	}
	PT3_PRINTK(7, KERN_DEBUG, "Allocate Descriptor buffer.\n");
	pt3_dma_build_page_descriptor(dma, 1);
	PT3_PRINTK(7, KERN_DEBUG, "set page descriptor.\n");
#if 0
	dma_check_page_descriptor(dma);
#endif

	return dma;
fail:
	if (dma != NULL)
#if defined(__FreeBSD__)
		free_pt3_dma(scp, dma);
#else
		free_pt3_dma(hwdev, dma);
#endif
	return NULL;
}

void
#if defined(__FreeBSD__)
free_pt3_dma(void *p, PT3_DMA *dma)
#else
free_pt3_dma(struct pci_dev *hwdev, PT3_DMA *dma)
#endif
{
	PT3_DMA_PAGE *page;
	__u32 i;
#if defined(__FreeBSD__)
	struct ptx_softc *scp;
	scp = p;
#endif
	if (dma->ts_info != NULL) {
		for (i = 0; i < dma->ts_count; i++) {
			page = &dma->ts_info[i];
			if (page->size != 0)
#ifdef __FreeBSD__
				free_dmabuf(page,scp->pt3_dmat);
#else
				pci_free_consistent(hwdev, page->size, page->data, page->addr);
#endif
		}
		kfree(dma->ts_info);
	}
	if (dma->desc_info != NULL) {
		for (i = 0; i < dma->desc_count; i++) {
			page = &dma->desc_info[i];
			if (page->size != 0)
#ifdef __FreeBSD__
				free_dmabuf(page,scp->dmat);
#else
				pci_free_consistent(hwdev, page->size, page->data, page->addr);
#endif
		}
		kfree(dma->desc_info);
	}
	kfree(dma);
}

#if defined(__FreeBSD__)
static uint8_t *
alloc_dmabuf(struct ptx_softc *scp, PT3_DMA_PAGE *page, bus_dma_tag_t dmat)
{
	int error;

	// allocate dma buffer
	error = bus_dmamem_alloc(dmat,
	    &page->va,
	    BUS_DMA_NOWAIT|BUS_DMA_ZERO,
	    &page->map);
	if (error) {
		device_printf(scp->device, 
			      "alloc_dmabuf: bus_dmamem_alloc returned %d\n", error);
		return 0;
	}

	// map dma buffer
	error = bus_dmamap_load(dmat,
	    page->map,
	    page->va,
	    page->size,
	    set_addr, &page->pa, // calback/arg
	    BUS_DMA_NOWAIT);
	if (error) {
		device_printf(scp->device,
			      "alloc_dmabuf: bus_dmamap_load returned %d\n", error);
		return 0;
	} else if (page->pa == 0) {
		device_printf(scp->device,
			      "alloc_dmabuf: bus_dmamap_load callback received %d size %d\n", EFBIG,page->size);
		error = EFBIG;
		return 0;
	}
	page->addr = (uintptr_t)page->pa;
	return (uint8_t *)page->va;
}
static void
set_addr(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{
	bus_addr_t *busaddrp = (bus_addr_t *)arg;

	if (error == 0)
		*busaddrp = segs[0].ds_addr;
	else
		*busaddrp = 0;
}
static void
free_dmabuf(PT3_DMA_PAGE *page, bus_dma_tag_t dmat)
{
	if (page->pa) {
		bus_dmamap_unload(dmat, page->map);
		bus_dmamem_free(dmat, page->va, page->map);

		memset(page, 0, sizeof(PT3_DMA_PAGE));
	}
}
#endif
