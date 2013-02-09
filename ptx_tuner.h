#ifndef _PTX_TUNER_H_
#define _PTX_TUNER_H_

extern int
ptx_tuner_init(struct ptx_softc *scp, int tuner);
extern void
settuner_reset(struct ptx_softc *scp, uint32_t lnb, uint32_t tuner);
void
set_sleepmode(struct ptx_softc *scp, struct ptx_stream *s, int type);
int
SetStream(struct ptx_softc *scp, int streamid, uint32_t enable);
int 
SetStreamGray(struct ptx_softc *scp, int streamid, uint32_t gray);
extern struct cdev*
ptx_make_tuner(int unit, int tuner, int chno);


int
SetFreq(struct ptx_softc *scp, struct ptx_stream *s, FREQUENCY *freq);
int
isdb_s_read_signal_strength(struct ptx_softc *scp, int streamid);
int
isdb_t_read_signal_strength(struct ptx_softc *scp, int streamid);

#endif
