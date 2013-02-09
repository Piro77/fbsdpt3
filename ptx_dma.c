#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include "ptx.h"

#include "ptx_dma.h"


static int
alloc_dmabuf(struct ptx_softc *scp, struct dmabuf *dmabuf, bus_dma_tag_t dmat);
static void
set_addr(void *arg, bus_dma_segment_t *segs, int nseg, int error);
static void
free_dmabuf(struct dmabuf *dmabuf, bus_dma_tag_t dmat);


int
ptx_dma_init(struct ptx_softc *scp)
{
	int rpos;
	int dpos;

	uint32_t *ptr;
	uintptr_t addr;
	uint32_t addr32;
	uint32_t *data;
	int error;

	// create address table
	for (rpos = 0; rpos < DMA_VIRTUAL_COUNTXSIZE; ++rpos) {
		error = alloc_dmabuf(scp, &scp->ringbuf[rpos], scp->dmat);
		if (error) {
			return error; // need freed
		}
	}

	// link address table
	for (rpos = 0; rpos < DMA_VIRTUAL_COUNTXSIZE - 1; ++rpos) {
		ptr = scp->ringbuf[rpos].va;
		addr = (uintptr_t) scp->ringbuf[rpos + 1].pa;
		addr >>= 12;
		addr32 = addr & 0x7fffffff;
		memcpy(ptr, &addr32, sizeof(uint32_t));
	}
	ptr = scp->ringbuf[rpos].va;
	addr = (uintptr_t) scp->ringbuf[0].pa;
	addr >>= 12;
	addr32 = addr & 0x7fffffff;
	memcpy(ptr, &addr32, sizeof(uint32_t));

	// create data buf
	for (rpos = 0; rpos < DMA_VIRTUAL_COUNTXSIZE; ++rpos) {
		ptr = scp->ringbuf[rpos].va;
		++ptr;

		for (dpos = 0; dpos < DMA_BUFFER_PAGE_COUNT; ++dpos) {
			error = alloc_dmabuf(scp, &scp->databuf[rpos][dpos], scp->dmat);
			if (error) {
				return error; // need freed
			}
			// DMAデータエリア初期化
			data = scp->databuf[rpos][dpos].va;
			data[(DMA_PAGE_SIZE / sizeof(uint32_t)) - 2] = 0;

			// table of page
			addr = (uintptr_t) scp->databuf[rpos][dpos].pa;
			addr >>= 12;
			addr32 = addr & 0x7fffffff;
			memcpy(ptr, &addr32, sizeof(uint32_t));

			++ptr;
		}
	}

	return 0 ;
}

static int
alloc_dmabuf(struct ptx_softc *scp, struct dmabuf *dmabuf, bus_dma_tag_t dmat)
{
	int error;

	// allocate dma buffer
	error = bus_dmamem_alloc(dmat,
	    &dmabuf->va,
	    BUS_DMA_NOWAIT|BUS_DMA_ZERO,
	    &dmabuf->map);
	if (error) {
		device_printf(scp->device, 
			      "alloc_dmabuf: bus_dmamem_alloc returned %d\n", error);
		return error;
	}

	// map dma buffer
	error = bus_dmamap_load(dmat,
	    dmabuf->map,
	    dmabuf->va,
	    DMA_PAGE_SIZE,
	    set_addr, &dmabuf->pa, // calback/arg
	    BUS_DMA_NOWAIT);
	if (error) {
		device_printf(scp->device,
			      "alloc_dmabuf: bus_dmamap_load returned %d\n", error);
	} else if (dmabuf->pa == 0) {
		device_printf(scp->device,
			      "alloc_dmabuf: bus_dmamap_load callback received %d\n", EFBIG);
		error = EFBIG;
	}

	return error;
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

int
ptx_dma_free(struct ptx_softc *scp)
{
	int rpos;
	int dpos;

	for (rpos = 0; rpos < DMA_VIRTUAL_COUNTXSIZE; ++rpos) {
		for (dpos = 0; dpos < DMA_BUFFER_PAGE_COUNT; ++dpos) {
			free_dmabuf(&scp->databuf[rpos][dpos], scp->dmat);
		}
		free_dmabuf(&scp->ringbuf[rpos], scp->dmat);
	}

	return 0;
}

static void
free_dmabuf(struct dmabuf *dmabuf, bus_dma_tag_t dmat)
{
	if (dmabuf->pa) {
		bus_dmamap_unload(dmat, dmabuf->map);
		bus_dmamem_free(dmat, dmabuf->va, dmabuf->map);

		memset(dmabuf, 0, sizeof(struct dmabuf));
	}
}
