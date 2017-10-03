#include <sys/param.h>
#include <sys/conf.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/bus.h>

#include <machine/bus.h>

#include "ptx.h"
#include "ptx_tuner.h"

#include "ptx_sysctl.h"

static int
sysctl_lnb(SYSCTL_HANDLER_ARGS);
static int
sysctl_freq(SYSCTL_HANDLER_ARGS);
static int
sysctl_signal(SYSCTL_HANDLER_ARGS);

void
ptx_sysctl_init(device_t device, struct ptx_softc *scp)
{
	struct sysctl_ctx_list *scl;
	struct sysctl_oid_list *sol;
	struct sysctl_oid *soid;

	scl = device_get_sysctl_ctx(device);
	sol = SYSCTL_CHILDREN(device_get_sysctl_tree(device));

	SYSCTL_ADD_PROC(scl, sol,
	    OID_AUTO, "lnb", CTLTYPE_INT|CTLFLAG_RW|CTLFLAG_ANYBODY,
	    scp, 0, sysctl_lnb, "I", "LNB");


	soid = SYSCTL_ADD_NODE(scl, sol,
	    OID_AUTO, "s0", CTLFLAG_RD, 0, "ISDB-S tuner0");

	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "freq", CTLTYPE_INT|CTLFLAG_WR|CTLFLAG_ANYBODY,
	    scp->dev[0], 0, sysctl_freq, "I", "channel freq.");
	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "signal", CTLTYPE_INT|CTLFLAG_RD,
	    scp->dev[0], 0, sysctl_signal, "I", "signal strength");

	soid = SYSCTL_ADD_NODE(scl, sol,
	    OID_AUTO, "t0", CTLFLAG_RD, 0, "ISDB-T tuner0");

	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "freq", CTLTYPE_INT|CTLFLAG_WR|CTLFLAG_ANYBODY,
	    scp->dev[1], 0, sysctl_freq, "I", "channel freq.");
	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "signal", CTLTYPE_INT|CTLFLAG_RD,
	    scp->dev[1], 0, sysctl_signal, "I", "signal strength");

	soid = SYSCTL_ADD_NODE(scl, sol,
	    OID_AUTO, "s1", CTLFLAG_RD, 0, "ISDB-S tuner1");

	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "freq", CTLTYPE_INT|CTLFLAG_WR|CTLFLAG_ANYBODY,
	    scp->dev[2], 0, sysctl_freq, "I", "channel freq.");
	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "signal", CTLTYPE_INT|CTLFLAG_RD,
	    scp->dev[2], 0, sysctl_signal, "I", "signal strength");

	soid = SYSCTL_ADD_NODE(scl, sol,
	    OID_AUTO, "t1", CTLFLAG_RD, 0, "ISDB-T tuner1");

	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "freq", CTLTYPE_INT|CTLFLAG_WR|CTLFLAG_ANYBODY,
	    scp->dev[3], 0, sysctl_freq, "I", "channel freq.");
	SYSCTL_ADD_PROC(scl, SYSCTL_CHILDREN(soid),
	    OID_AUTO, "signal", CTLTYPE_INT|CTLFLAG_RD,
	    scp->dev[3], 0, sysctl_signal, "I", "signal strength");
}

static int
sysctl_lnb(SYSCTL_HANDLER_ARGS)
{
	struct ptx_softc *scp = (struct ptx_softc *)arg1;

	int val = scp->lnb;
	int error;

	error = sysctl_handle_int(oidp, &val, 0, req);
	if (error) {
		return (error);
	}

	if (val != scp->lnb) {
		// LNB OFF:0 +11V:1 +15V:2
		if (0 <= val && val <= 2) {
			if (val) {
				settuner_reset(scp, val, TUNER_POWER_ON_RESET_DISABLE);
			} else {
				settuner_reset(scp, LNB_OFF, TUNER_POWER_ON_RESET_DISABLE);
			}
			scp->lnb = val;
		} else {
			error = EINVAL;
		}
	}

	return error;
}

static int
sysctl_freq(SYSCTL_HANDLER_ARGS)
{
	struct cdev *dev = (struct cdev *)arg1;
	struct ptx_softc *scp = (struct ptx_softc *)dev->si_drv1;
	struct ptx_stream *s = (struct ptx_stream *)dev->si_drv2;

	FREQUENCY freq;
	int error;

	error = sysctl_handle_int(oidp, &s->freq, 0, req);
	if (error || !req->newptr) {
		return (error);
	}

	freq.frequencyno = s->freq & 0xffff;
	freq.slot = (s->freq >> 16)& 0xffff;
	return SetFreq(scp, s, &freq);
}

static int
sysctl_signal(SYSCTL_HANDLER_ARGS)
{
	struct cdev *dev = (struct cdev *)arg1;
	struct ptx_softc *scp = (struct ptx_softc *)dev->si_drv1;
	struct ptx_stream *s = (struct ptx_stream *)dev->si_drv2;

	int val;

	if (req->newptr) {
		return EPERM;
	}

	switch (s->id) {
	case 1:
	case 3:
		val = isdb_s_read_signal_strength(scp, s->id);
		break;
	case 2:
	case 4:
		val = isdb_t_read_signal_strength(scp, s->id);
		break;

	default:
		return EIO;
	}

	return(sysctl_handle_int(oidp, NULL, val, req));
}
