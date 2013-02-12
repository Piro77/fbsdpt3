#ifndef		__PT3_MISC_H__
#define		__PT3_MISC_H__

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/types.h>
#include <sys/uio.h>


#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>

#define	ssize_t	size_t

#define	pt3_vzalloc(A)	malloc(A,M_DEVBUF,M_NOWAIT)
#define	vzalloc(A)	malloc(A,M_DEVBUF,M_NOWAIT)
#define kzalloc(A,B)	malloc(A,M_DEVBUF,M_NOWAIT)
#define vfree(A)	free(A,M_DEVBUF)
#define kfree(A)	free(A,M_DEVBUF)

#define __u32 uint32_t
#define __u8 uint8_t
#define __u16 uint16_t
#define __u64 uint64_t
#define __s32 int32_t
#define dma_addr_t uint32_t
#define	__user
#define loff_t off_t

#define likely(a) a
#define unlikely(a) a

#define INIT_DUMMY_RESET 0x0c

#define msecs_to_jiffies(ms)		MSTOTICK(ms)
#define	schedule_timeout_interruptible(A)	ptx_pause("pt3",A)
#define	do_gettimeofday(time)		microtime(time)

#include "pt3_com.h"
#include "pt3_pci.h"
#include "pt3_bus.h"
#include "pt3_i2c.h"
#include "pt3_tc.h"
#include "pt3_qm.h"
#include "pt3_mx.h"
#include "pt3_dma.h"

typedef struct _PT3_TUNER {
        int tuner_no;
        PT3_TC *tc_s;
        PT3_TC *tc_t;
        PT3_QM *qm;
        PT3_MX *mx;
} PT3_TUNER;

typedef struct _PT3_CHANNEL PT3_CHANNEL;


struct _PT3_CHANNEL {
        __u32                   valid ;
        __u32                   minor;
        PT3_TUNER               *tuner;
        int                             type ;
        struct mtx    lock ;
        PT3_I2C                 *i2c;
        PT3_DMA                 *dma;
	int	id;
	int	freq;
	struct cv not_full;
	struct cv not_empty;
};

#include "ptx.h"

#endif

