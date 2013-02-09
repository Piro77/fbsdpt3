#include <sys/param.h>
#include <sys/conf.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/unistd.h>
#include <sys/kthread.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include "ptx.h"
#include "ptx_tuner.h"

#include "ptx_proc.h"

static void
ptx_daemon(void* data);
static void
read_dmabuf(struct ptx_softc *scp, uint32_t *dataptr);
static void
reset_dma(struct ptx_softc *scp);

int
ptx_proc_start(struct ptx_softc *scp)
{
#if (__FreeBSD_version < 800000)
	int err = kthread_create(ptx_daemon, scp, &scp->ptxdaemon,
	    RFTHREAD, 0, "ptxdaemon");
#else
	int err = kthread_add(ptx_daemon, scp, NULL,
	    &scp->ptxdaemon, RFTHREAD, 0, "ptxdaemon");
#endif
	if (err) {
		device_printf(scp->device, "ptx_proc_start: kthread_add retured %d\n", err);
		return ENXIO;
	}

	scp->ring_pos = 0;
	scp->data_pos = 0;
	scp->procrun = TRUE;
	return 0;
}

void
ptx_proc_stop(struct ptx_softc *scp)
{
	scp->procrun = FALSE;
}

static void
ptx_daemon(void* data)
{
	struct ptx_softc *scp = (struct ptx_softc *) data;
//	struct ptx_stream *s;

	uint32_t *dataptr;

	reset_dma(scp);

	while (scp->procrun == TRUE) {
		while (scp->procrun == TRUE) {

			dataptr = scp->databuf[scp->ring_pos][scp->data_pos].va;

			// データあり？
			if (dataptr[(DMA_PAGE_SIZE / sizeof(uint32_t)) - 2] == 0) {
				break;
			}

			scp->data_pos += 1;

			read_dmabuf(scp, dataptr);

			dataptr[(DMA_PAGE_SIZE / sizeof(uint32_t)) - 2] = 0;

			if (scp->data_pos >= DMA_BUFFER_PAGE_COUNT) {
				scp->data_pos = 0;
				scp->ring_pos += 1;
				// DMAリングが変わった場合はインクリメント
				bus_space_write_4(scp->bt, scp->bh, 0x0, 0x00000020);
				if (scp->ring_pos >= DMA_VIRTUAL_COUNTXSIZE) {
					scp->ring_pos = 0;
				}
			}
		}
		ptx_pause("ptxdmn", MSTOTICK(1));
	}

#if (__FreeBSD_version < 800000)
	kthread_exit(0);
#else
	kthread_exit();
#endif
}

static void
read_dmabuf(struct ptx_softc *scp, uint32_t *dataptr)
{
	struct ptx_stream *s;

	int lp;

	uint32_t errval;

	union {
		uint32_t val;
		MICRO_PACKET packet;
	} micro;

	uint8_t id;
	uint8_t count;
	uint8_t st;
	uint8_t er;

	for (lp = 0; lp < (DMA_PAGE_SIZE / sizeof(uint32_t)); lp++, dataptr++) {
		micro.val = *dataptr;

		id    = (micro.packet.head >> 5) & 0x7;
		count = (micro.packet.head >> 2) & 0x7;
		st    = (micro.packet.head >> 1) & 0x1;
		er    = (micro.packet.head >> 0) & 0x1;

		//チャネル情報不正
		if (id > MAX_STREAM || id == 0) {
			device_printf(scp->device, "DMA Channel Number Error(%d)\n", id);
			continue;
		}

		s = &scp->stream[id - 1];

		//  エラーチェック
		if (er) {
			device_printf(scp->device, "stream %d micropacket error\n", id);

			errval = bus_space_read_4(scp->bt, scp->bh, 0x0);
			if (errval & BIT_RAM_OVERFLOW) {
				s->overflow += 1;
			}
			if (errval & BIT_INITIATOR_ERROR) {
				s->counetererr += 1;
			}
			if (errval & BIT_INITIATOR_WARNING) {
				s->transerr += 1;
			}
#if 1
			// 初期化して先頭から
			reset_dma(scp);
			scp->ring_pos = scp->data_pos = 0;
			break;
#else
			continue;
#endif
		}

		// 未使用チャネルは捨てる
		if (s->opened == FALSE) {
			continue;
		}

		// 先頭で、一時バッファの残りは捨てる
		if (st && (s->chunk_filled % PACKET_SIZE != 0)) {
			int mod = s->chunk_filled % PACKET_SIZE;
			s->chunk_filled -= mod;
		}

		if (s->chunk_filled >= DATA_CHUNK_SIZE) {
			// 次の書き込み先を確保

			mtx_lock(&s->lock);
			if ( ((s->wp + 1) % DATA_CHUNK_NUM) == s->rp ) {
				// 対象チャンネルのバッファがあふれた場合
				cv_timedwait(&s->not_full, &s->lock, MSTOTICK(500));
			}
			if ( ((s->wp + 1) % DATA_CHUNK_NUM) == s->rp ) {
				mtx_unlock(&s->lock);
				s->drop += 1;
				continue;
			}

			s->wp++;
			if (s->wp >= DATA_CHUNK_NUM)
				s->wp = 0;
			cv_signal(&s->not_empty);
			mtx_unlock(&s->lock);

			s->chunk_filled = 0;
		}

		// データコピー
		KASSERT(s->chunk_filled >= 0, "negative chunk_filled?");
		KASSERT(s->chunk_filled < DATA_CHUNK_SIZE, "chunk_filled overflow");
		KASSERT(s->wp >= 0, "negative wp?");
		KASSERT(s->wp < DATA_CHUNK_NUM, "wp overflow");
		s->buf[s->wp * DATA_CHUNK_SIZE + s->chunk_filled] = micro.packet.data[2];
		s->chunk_filled++;
		s->buf[s->wp * DATA_CHUNK_SIZE + s->chunk_filled] = micro.packet.data[1];
		s->chunk_filled++;
		if (s->chunk_filled % PACKET_SIZE != 0) {
			s->buf[s->wp * DATA_CHUNK_SIZE + s->chunk_filled] = micro.packet.data[0];
			s->chunk_filled++;
		} else {
			// 189byte目はinformation
//printf("ptx: proc stream %d info=%02x\n", s->id, micro.packet.data[0]);
		}
	}
}

static void
reset_dma(struct ptx_softc *scp)
{
	int rpos;
	int dpos;
	uintptr_t addr;
	uint32_t addr32;
	uint32_t *dataptr;

	// データ初期化
	for (rpos = 0; rpos < DMA_VIRTUAL_COUNTXSIZE; ++rpos) {
		for (dpos = 0; dpos < DMA_BUFFER_PAGE_COUNT; ++dpos) {
			dataptr = scp->databuf[rpos][dpos].va;
			dataptr[(DMA_PAGE_SIZE / sizeof(uint32_t)) - 2] = 0;
		}
	}

	// 転送カウンタをリセット
	bus_space_write_4(scp->bt, scp->bh, 0x00, 0x00000010);
	// 転送カウンタをインクリメント
	for (rpos = 0; rpos < DMA_VIRTUAL_COUNTXSIZE; ++rpos) {
		bus_space_write_4(scp->bt, scp->bh, 0x00, 0x00000020);
	}

	addr = (uintptr_t) scp->ringbuf[0].pa;
	addr >>= 12;
	addr32 = addr & 0x7fffffff;
	// DMAバッファ設定
	bus_space_write_4(scp->bt, scp->bh, DMA_ADDR, addr32);
	// DMA開始
	bus_space_write_4(scp->bt, scp->bh, 0x00, 0x0c000040);
}
