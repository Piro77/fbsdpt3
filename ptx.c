#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/condvar.h>

#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include "pt3_misc.h"

#include "ptx.h"
#include "ptx_iic.h"
#include "ptx_tuner.h"
#include "ptx_dma.h"
#include "ptx_proc.h"
#include "ptx_sysctl.h"




static devclass_t ptx_devclass;

/*
 ***************************************
 * PCI Attachment structures and code
 ***************************************
 */

static int ptx_probe(device_t);
static int ptx_attach(device_t);
static int ptx_detach(device_t);

static device_method_t ptx_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		ptx_probe),
	DEVMETHOD(device_attach,	ptx_attach),
	DEVMETHOD(device_detach,	ptx_detach),

	{ 0, 0 }
};

static driver_t ptx_driver = {
	"ptx",
	ptx_methods,
	sizeof(struct ptx_softc),
};

DRIVER_MODULE(ptx, pci, ptx_driver, ptx_devclass, 0, 0);
MODULE_VERSION(ptx, 1);

#define VENDOR_XILINX 0x10ee
#define PCI_PT1_ID 0x211a
#define PCI_PT2_ID 0x222a
#define VENDOR_ALTERA 0x1172
#define PCI_PT3_ID 0x4c15

/*
 * device interface
 */
static int
ptx_probe(device_t device)
{
	uint32_t type = pci_get_devid(device);
	struct _pcsid
	{
		uint32_t type;
		const char *desc;
	};
	static const struct _pcsid pci_ids[] = {
		{ (PCI_PT1_ID << 16)|VENDOR_XILINX, "EARTHSOFT PT1" },
		{ (PCI_PT2_ID << 16)|VENDOR_XILINX, "EARTHSOFT PT2" },
		{ (PCI_PT3_ID << 16)|VENDOR_ALTERA, "EARTHSOFT PT3" },

		{ 0x00000000, NULL }
	};
	struct _pcsid const *ep = pci_ids;

	while (ep->type && ep->type != type)
		++ep;
	if (ep->desc) {
		device_set_desc(device, ep->desc);
		return BUS_PROBE_SPECIFIC; /* If there might be a better driver, return -2 */
	} else
		return ENXIO;
}

static int
ptx_attach(device_t device)
{
	struct ptx_softc *scp = (struct ptx_softc *) device_get_softc(device);

	int tuner;
	int i;

	uint32_t command;
        int error;

	memset(scp, 0, sizeof(struct ptx_softc));

	scp->unit = device_get_unit(device);
	scp->device = device;

	// Enable bus mastering and memory mapped I/O.
	pci_enable_busmaster(device);
	pci_enable_io(device, SYS_RES_MEMORY);
	command = pci_read_config(device, PCIR_COMMAND, 2);
	if (! (command & PCIM_CMD_BUSMASTEREN)) {
		device_printf(device, "Can't enable PCI busmaster\n");
		return ENXIO;
	}
	if (! (command & PCIM_CMD_MEMEN)) {
		device_printf(device, "failed to enable memory mappings\n");
		return ENXIO;
	}

	switch (pci_get_device(device)) {
	case PCI_PT1_ID:
		scp->cardtype = PT1;
		break;
	case PCI_PT2_ID:
		scp->cardtype = PT2;
		break;
	case PCI_PT3_ID:
		scp->cardtype = PT3;
		break;
	default:
		device_printf(device, "unknown device (0x%04x)\n",
		    pci_get_device(device));
		return ENXIO;
	}

	// PCIアドレスをマップする
	scp->rid_memory = PCIR_BARS;
	scp->res_memory = bus_alloc_resource_any(device, SYS_RES_MEMORY,
	    &scp->rid_memory, RF_ACTIVE);
	if (! scp->res_memory) {
		device_printf(device, "could not map memory\n");
		return ENXIO;
	}
	scp->bt = rman_get_bustag(scp->res_memory);
	scp->bh = rman_get_bushandle(scp->res_memory);

if (scp->cardtype == PT3) {
if (pt3_init(scp)) {
		device_printf(device, "Error pt3_init\n");
		goto out_err;
}
}
else {
	if (xc3s_init(scp)) {
		device_printf(device, "Error xc3s_init\n");
		goto out_err;
	}

	// チューナリセット
	scp->lnb = 0;

	settuner_reset(scp, LNB_OFF, TUNER_POWER_ON_RESET_ENABLE);
	ptx_pause("ptxini", MSTOTICK(50));

	settuner_reset(scp, LNB_OFF, TUNER_POWER_ON_RESET_DISABLE);
	ptx_pause("ptxini", MSTOTICK(10));

	mtx_init(&scp->lock, "ptxiic", NULL, MTX_DEF);
	scp->i2c_state = STATE_STOP;
	scp->i2c_progress = 0;

	// Tuner/Stream 初期化処理
	for (tuner = 0; tuner < 2; ++tuner) {
		if (ptx_tuner_init(scp, tuner))
			goto out_err;

		// |  1 | チューナー番号0 ISDB-S |
		// |  2 | チューナー番号0 ISDB-T |
		// |  3 | チューナー番号1 ISDB-S |
		// |  4 | チューナー番号1 ISDB-T |
		for (i = 0; i < 2; ++i) {
			// 0=ISDB-S 1=ISDB-T
			struct ptx_stream *s = &scp->stream[tuner*2+i];

			s->id = tuner*2+i + 1;

			s->wp = 0;
			s->chunk_filled = 0;
			s->rp = 0;
			s->chunk_used = 0;

			s->buf = malloc(DATA_CHUNK_SIZE * DATA_CHUNK_NUM,
			    M_DEVBUF, M_NOWAIT);
			if (s->buf == NULL) {
				device_printf(scp->device, "malloc failed\n");
				goto out_err;
			}
			mtx_init(&s->lock, "ptxstream", NULL, MTX_DEF);
			cv_init(&s->not_full, "ptxful");
			cv_init(&s->not_empty, "ptxemp");

			scp->dev[s->id - 1] = ptx_make_tuner(scp->unit, tuner, i);
			scp->dev[s->id - 1]->si_drv1 = scp;
			scp->dev[s->id - 1]->si_drv2 = s;

			set_sleepmode(scp, s, TYPE_SLEEP);
		}
		ptx_pause("ptxini", MSTOTICK(50));
	}

	/*
	 * Allocate a DMA tag for the parent bus.
	 */
	error = bus_dma_tag_create(NULL,
	    4, 0, // alignment=4byte, boundary=norestriction
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR,
	    NULL, NULL, // no filtfunc/arg
	    DMA_PAGE_SIZE, 1, DMA_PAGE_SIZE, // maxsize, 1segs, segsize
	    0,
	    NULL, NULL, // no lockfunc/arg
	    &scp->dmat);
	if (error) {
		device_printf(device, "could not create bus DMA tag(%d)\n", error);
		goto out_err;
	}

	ptx_sysctl_init(device, scp);

	if (ptx_dma_init(scp)) {
		goto out_err;
	}

	if (ptx_proc_start(scp)) {
		goto out_err;
	}
}

	return 0;

	// ----------------
out_err:
	ptx_detach(device);
	return ENXIO;
}

static int
ptx_detach (device_t device)
{
	struct ptx_softc *scp = (struct ptx_softc *) device_get_softc(device);
	uint32_t val;
	int lp;

	if (scp->cardtype == PT3) {

	pt3_exit(scp);

	if (scp->res_memory) {
		bus_release_resource(device, SYS_RES_MEMORY,
			scp->rid_memory, scp->res_memory);
		scp->res_memory = 0;
	}
	if (scp->pt3_res_memory) {
		bus_release_resource(device, SYS_RES_MEMORY,
			scp->pt3_rid_memory, scp->pt3_res_memory);
		scp->pt3_res_memory = 0;
	}

		return 0;
	}

	if (scp->ptxdaemon) {
		ptx_proc_stop(scp);
	}

	// DMA終了
	if (scp->bh) {
		bus_space_write_4(scp->bt, scp->bh, 0x0, 0x08080000);
		for (lp = 0; lp < 10; lp++){
			val = bus_space_read_4(scp->bt, scp->bh, 0x0);
			if (!(val & (1 << 6))) {
				break;
			}
			ptx_pause("ptxdet", MSTOTICK(1));
		}
	}

	ptx_dma_free(scp);

	for (lp = 0; lp < MAX_STREAM; ++lp) {
		struct ptx_stream *s = &scp->stream[lp];
		if (mtx_initialized(&s->lock)) {
			mtx_destroy(&s->lock);
			cv_destroy(&s->not_full);
			cv_destroy(&s->not_empty);
		}
		if (s->buf != NULL) {
			free(s->buf, M_DEVBUF);
			s->buf = NULL;
		}

		if (scp->dev[lp]) {
			destroy_dev(scp->dev[lp]);
		}
	}

	if (scp->bh) {
		bus_space_write_4(scp->bt, scp->bh, 0x0, 0xb0b0000);
		bus_space_write_4(scp->bt, scp->bh, CFG_REGS_ADDR, 0x0);
	}
	settuner_reset(scp, LNB_OFF, TUNER_POWER_OFF);

	if (scp->dmat) {
		bus_dma_tag_destroy(scp->dmat);
	}

	if (mtx_initialized(&scp->lock)) {
		mtx_destroy(&scp->lock);
	}

	if (scp->res_memory) {
		bus_release_resource(device, SYS_RES_MEMORY,
			scp->rid_memory, scp->res_memory);
		scp->res_memory = 0;
	}

	return 0;
}

int
ptx_pause(const char *wmesg, int timo)
{
	if (cold) {
		/* convert to milliseconds */
		timo = (timo * 1000) / hz;
		/* convert to microseconds, rounded up */
		timo = (timo + 1) * 1000;
		DELAY(timo);
		return EWOULDBLOCK;
	} else
		return pause(wmesg, timo);
}
