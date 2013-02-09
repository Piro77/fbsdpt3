#ifndef _PTX_IIC_H_
#define _PTX_IIC_H_

extern int
xc3s_init(struct ptx_softc *);
extern void
i2c_write(struct ptx_softc *scp, WBLOCK *wblock);
extern uint32_t
i2c_read(struct ptx_softc *scp, WBLOCK *wblock, int size);

#endif
