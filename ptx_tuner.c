#include <sys/param.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include "ptx.h"
#include "ptx_iic.h"

#include "ptx_tuner_data.h"
#include "ptx_tuner.h"

typedef	struct _isdb_t_freq_add_table {
	uint16_t pos;		// 追加するチャンネルポジション
	uint16_t add_freq;	// 追加する値
} isdb_t_freq_add_table;

static const isdb_t_freq_add_table isdb_t_freq_add[10] = {
	{  7, 0x8081},				// 0〜7迄
	{ 12, 0x80a1},				// 8〜12迄
	{ 21, 0x8062},				// 13〜21迄
	{ 39, 0x80a2},				// 22〜39迄
	{ 51, 0x80e2},				// 40〜51迄
	{ 59, 0x8064},				// 52〜59迄
	{ 75, 0x8084},				// 60〜75迄
	{ 84, 0x80a4},				// 76〜84迄
	{100, 0x80c4},				// 85〜100迄
	{112, 0x80e4}				// 101〜112迄
};

static int
init_isdb_s(struct ptx_softc *scp, uint8_t addr);
static void
init_isdb_t(struct ptx_softc *scp, uint8_t addr);

static int
ts_lock(struct ptx_softc *scp, uint8_t addr, uint16_t ts_id);
static int
bs_tune(struct ptx_softc *scp, uint8_t addr, int16_t channel, ISDB_S_TMCC *tmcc);
static int
bs_frequency(struct ptx_softc *scp, uint8_t addr, int16_t channel);

static int
isdb_t_frequency(struct ptx_softc *scp, uint8_t addr, int16_t channel, int addfreq);
static uint16_t
getfrequency_add(int16_t channel);
static uint16_t
getfrequency(int16_t channel, uint16_t addfreq);

static uint8_t
geti2c(int streamid);
static int
getchtype(int streamid);


int
ptx_tuner_init(struct ptx_softc *scp, int tuner_no)
{
	typedef struct _TUNER_INFO {
		uint8_t isdb_s;
		uint8_t isdb_t;
	} TUNER_INFO;

	static TUNER_INFO tuner_info[2] = {
		{T0_ISDB_S, T0_ISDB_T},
		{T1_ISDB_S, T1_ISDB_T}
	};

	int rc;
	WBLOCK wk;

	// ISDB-S/T初期化
	memcpy(&wk, &com_initdata, sizeof(WBLOCK));

	// 初期化(共通)
	wk.addr = tuner_info[tuner_no].isdb_t;
	i2c_write(scp, &wk);
	wk.addr = tuner_info[tuner_no].isdb_s;
	i2c_write(scp, &wk);

	rc = init_isdb_s(scp, tuner_info[tuner_no].isdb_s);
	if (rc) {
		return rc;
	}
	init_isdb_t(scp, tuner_info[tuner_no].isdb_t);

	memcpy(&wk, &isdb_s_init21, sizeof(WBLOCK));
	wk.addr = tuner_info[tuner_no].isdb_s;
	i2c_write(scp, &wk);

	memcpy(&wk, &isdb_t_init17, sizeof(WBLOCK));
	wk.addr = tuner_info[tuner_no].isdb_t;
	i2c_write(scp, &wk);

	return 0;
}

static int
init_isdb_s(struct ptx_softc *scp, uint8_t addr)
{
	WBLOCK wk;
	int lp;
	uint32_t val;
#if 0
	// ISDB-S/T初期化
	memcpy(&wk, &com_initdata, sizeof(WBLOCK));
#endif
	// 初期化１(なぜかREADなので)
	memcpy(&wk, &isdb_s_init1, sizeof(WBLOCK));
	wk.addr = addr;
	val = i2c_read(scp, &wk, 1);

	if (scp->cardtype == PT1) {
		if ((val & 0xff) != 0x4c) {
			device_printf(scp->device, "ISDB-S Read(%x)\n", val);
			return EIO;
		}
		for (lp = 0; lp < PT1_MAX_ISDB_S_INIT; lp++) {
			memcpy(&wk, isdb_s_initial_pt1[lp], sizeof(WBLOCK));
			wk.addr = addr;
			i2c_write(scp, &wk);
		}
	} else if (scp->cardtype == PT2) {
		if ((val & 0xff) != 0x52) {
			device_printf(scp->device, "ISDB-S Read(%x)\n", val);
			return EIO;
		}
		for (lp = 0; lp < PT2_MAX_ISDB_S_INIT; lp++) {
			memcpy(&wk, isdb_s_initial_pt2[lp], sizeof(WBLOCK));
			wk.addr = addr;
			i2c_write(scp, &wk);
		}
	}

	return 0;
}

static void
init_isdb_t(struct ptx_softc *scp, uint8_t addr)
{
	int lp;
	WBLOCK wk;

	// ISDB-S/T初期化
	if (scp->cardtype == PT1) {
		for (lp = 0; lp < PT1_MAX_ISDB_T_INIT; lp++) {
			memcpy(&wk, isdb_t_initial_pt1[lp], sizeof(WBLOCK));
			wk.addr = addr;
			i2c_write(scp, &wk);
		}
	} else if (scp->cardtype == PT2) {
		for (lp = 0; lp < PT2_MAX_ISDB_T_INIT; lp++) {
			memcpy(&wk, isdb_t_initial_pt2[lp], sizeof(WBLOCK));
			wk.addr = addr;
			i2c_write(scp, &wk);
		}
	}
}

void
set_sleepmode(struct ptx_softc *scp, struct ptx_stream *s, int type)
{
	WBLOCK wk;

	int chtype = getchtype(s->id);
	uint8_t i2cadr = geti2c(s->id);

	if (chtype == CHANNEL_TYPE_ISDB_S
	    && type == TYPE_WAKEUP) {
		//printf("PT1:ISDB-S Wakeup\n");
		memcpy(&wk, &isdb_s_wake, sizeof(WBLOCK));
		wk.addr = i2cadr;
		i2c_write(scp, &wk);

		memcpy(&wk, &isdb_s_sleep, sizeof(WBLOCK));
		wk.value[1] = 0x01;
		wk.addr = i2cadr;
		i2c_write(scp, &wk);
	} else if (chtype == CHANNEL_TYPE_ISDB_S
	    && type == TYPE_SLEEP) {
		//printf("PT1:ISDB-S Sleep\n");
		memcpy(&wk, &isdb_s_sleep, sizeof(WBLOCK));
		wk.addr = i2cadr;
		i2c_write(scp, &wk);
	} else if (chtype == CHANNEL_TYPE_ISDB_T
	    && type == TYPE_WAKEUP) {
		//printf("PT1:ISDB-T Wakeup\n");
		memcpy(&wk, &isdb_t_wake, sizeof(WBLOCK));
		wk.addr = i2cadr;
		i2c_write(scp, &wk);

		memcpy(&wk, &isdb_t_sleep, sizeof(WBLOCK));
		wk.value[1] = 0x90;
		wk.addr = i2cadr;
		i2c_write(scp, &wk);
	} else if (chtype == CHANNEL_TYPE_ISDB_T
	    && type == TYPE_SLEEP) {
		//printf("PT1:ISDB-T Sleep\n");
		memcpy(&wk, &isdb_t_sleep, sizeof(WBLOCK));
		wk.addr = i2cadr;
		i2c_write(scp, &wk);
	}
}

/*
 character device driver routines
 */
static d_open_t ptxopen;
static d_close_t ptxclose;
static d_read_t ptxread;
//static d_ioctl_t ptxioctl;

static struct cdevsw ptx_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	ptxopen,
	.d_close =	ptxclose,
	.d_read =	ptxread,
//	.d_ioctl =	ptxioctl,
	.d_name =	"ptx_tuner",
};

struct cdev*
ptx_make_tuner(int unit, int tuner, int chno)
{
	static const char* fmt[] = {
		"ptx%d.s%d",
		"ptx%d.t%d"};

	return make_dev(&ptx_cdevsw, 0,
	    UID_ROOT, GID_OPERATOR, 0666,
	    fmt[chno], unit, tuner);
}

/*
static int
ptxioctl(struct cdev *dev, u_long cmd, caddr_t data, int flag, struct thread *td)
{
	struct ptx_softc *scp = (struct ptx_softc *) dev->si_drv1;
	struct ptx_stream *s = (struct ptx_stream *) dev->si_drv2;

	int lnb_eff, lnb_usr;
	FREQUENCY freq;
	//char *voltage[] = {"0V", "11V", "15V"};

	switch (cmd) {
	case SET_CHANNEL:
		memcpy(&freq, (FREQUENCY *)data, sizeof(FREQUENCY));
		return SetFreq(scp, s, &freq);

	case START_REC:
		SetStream(scp, s->id, TRUE);
		return 0;

	case STOP_REC:
		SetStream(scp, s->id, FALSE);
		return 0;

	case GET_SIGNAL_STRENGTH:
		switch (getchtype(s->id)) {
		case CHANNEL_TYPE_ISDB_S:
			*((int *) data) = isdb_s_read_signal_strength(scp, s->id);
			break;
		case CHANNEL_TYPE_ISDB_T:
			*((int *) data) = isdb_t_read_signal_strength(scp, s->id);
			break;
		}
		return 0;

	case LNB_ENABLE:
		lnb_usr = *(int*)data;
		lnb_eff = lnb_usr ? lnb_usr : scp->lnb;
		mtx_lock(&scp->lnb_mutex);
		scp->lnb_ref_count += 1;
		settuner_reset(scp, lnb_eff, TUNER_POWER_ON_RESET_DISABLE);
		mtx_unlock(&scp->lnb_mutex);
		//printf("PT1:LNB = %s\n", voltage[lnb_eff]);
		//printf("PT1:LNB ref_count = %d\n", scp->lnb_ref_count);
		return 0;

	case LNB_DISABLE:
		mtx_lock(&scp->lnb_mutex);
		scp->lnb_ref_count -= 1;
		if (! scp->lnb_ref_count) {
			settuner_reset(scp, LNB_OFF, TUNER_POWER_ON_RESET_DISABLE);
		}
		mtx_unlock(&scp->lnb_mutex);
		//printf("PT1:LNB ref_count = %d\n", scp->lnb_ref_count);
		return 0;
	}

	return EINVAL;
}
*/

static int
ptxopen(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
	//struct ptx_softc *scp = (struct ptx_softc *) dev->si_drv1;
	struct ptx_stream *s = (struct ptx_stream *) dev->si_drv2;

	mtx_lock(&s->lock);

	if (s->opened == TRUE) {
		mtx_unlock(&s->lock);
		return EBUSY;
	}

	s->opened = TRUE;
	s->started = FALSE;

	s->drop = 0;
	s->overflow = 0;
	s->counetererr = 0;
	s->transerr = 0;

	s->wp = 0;
	s->chunk_filled = 0;
	s->rp = 0;
	s->chunk_used = 0;

	mtx_unlock(&s->lock);
	return 0;
}

static int
ptxclose(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
	struct ptx_softc *scp = (struct ptx_softc *) dev->si_drv1;
	struct ptx_stream *s = (struct ptx_stream *) dev->si_drv2;

	mtx_lock(&s->lock);
	SetStream(scp, s->id, FALSE);
	s->started = FALSE;
	s->opened = FALSE;

//	printf("(%s)Drop=%08d:%08d:%08d:%08d\n",
//	    dev->si_name,
//	    s->drop, s->overflow, s->counetererr, s->transerr);
	s->overflow = 0;
	s->counetererr = 0;
	s->transerr = 0;
	s->drop = 0;

	// 停止している場合は起こす
	cv_signal(&s->not_full);

	mtx_unlock(&s->lock);
	return 0;
}

static int
ptxread(struct cdev *dev, struct uio *uio, int ioflag)
{
	struct ptx_softc *scp = (struct ptx_softc *) dev->si_drv1;
	struct ptx_stream *s = (struct ptx_stream *) dev->si_drv2;

	int resid;
	int available;
	int size;
	int rv;

	if (s->started == FALSE) {
		SetStream(scp, s->id, TRUE);
		s->started = TRUE;
	}

	mtx_lock(&s->lock);
	while (s->rp == s->wp) {
		cv_wait(&s->not_empty, &s->lock);

		if (s->opened == FALSE || s->started == FALSE) {
			mtx_unlock(&s->lock);
			return -1;
		}
	}
	mtx_unlock(&s->lock);

	resid = uio->uio_resid;
	if (s->chunk_used < DATA_CHUNK_SIZE) {
		// 転送残り
		available = DATA_CHUNK_SIZE - s->chunk_used;
		size = (resid < available) ? resid : available;
		rv = uiomove(&s->buf[s->rp * DATA_CHUNK_SIZE + s->chunk_used], size, uio);
		if (rv)
			return rv;

		s->chunk_used += size;
		KASSERT(s->chunk_used <= DATA_CHUNK_SIZE, "chunk_used overflow");
		resid -= size;
		KASSERT(resid >= 0, "resid underflow");
	}

	while (resid > 0) {

		if (s->chunk_used >= DATA_CHUNK_SIZE) {
			// 次の読みこみ先を確保

			mtx_lock(&s->lock);
			s->rp++;
			if (s->rp >= DATA_CHUNK_NUM)
				s->rp = 0;
			cv_signal(&s->not_full);

			while (s->rp == s->wp) {
				cv_wait(&s->not_empty, &s->lock);

				if (s->opened == FALSE || s->started == FALSE) {
					mtx_unlock(&s->lock);
					return -1;
				}
			}

			mtx_unlock(&s->lock);

			s->chunk_used = 0;
		}

		available = DATA_CHUNK_SIZE - s->chunk_used;
		size = (resid < available) ? resid : available;
		rv = uiomove(&s->buf[s->rp * DATA_CHUNK_SIZE + s->chunk_used], size, uio);
		if (rv)
			return rv;

		s->chunk_used += size;
		KASSERT(s->chunk_used <= DATA_CHUNK_SIZE, "chunk_used overflow");
		resid -= size;
		KASSERT(resid >= 0, "resid underflow");
	}

	return 0;
}

/*

 */
int
SetFreq(struct ptx_softc *scp, struct ptx_stream *s, FREQUENCY *freq)
{
	ISDB_S_TMCC tmcc;
	int error;

	switch (getchtype(s->id)) {
	case CHANNEL_TYPE_ISDB_S:
		error = bs_tune(scp, geti2c(s->id), freq->frequencyno, &tmcc);
		if (error) {
			return error;
		}
		error = ts_lock(scp, geti2c(s->id), tmcc.ts_id[freq->slot].ts_id);
		if (error) {
			return error;
		}
		break;

	case CHANNEL_TYPE_ISDB_T:
		error = isdb_t_frequency(scp, geti2c(s->id), freq->frequencyno, freq->slot);
		if (error) {
			return error;
		}
	}

	return 0;
}

int
SetStream(struct ptx_softc *scp, int streamid, uint32_t enable)
{
	int index = streamid - 1;

	uint32_t val = (1 << (8 + index));
	if (enable) {
		val |= (1 << index);
	}
	bus_space_write_4(scp->bt, scp->bh, TS_TEST_ENABLE_ADDR, val);
	return 0;
}

int 
SetStreamGray(struct ptx_softc *scp, int streamid, uint32_t gray)
{
	if (8 <= gray)
	  return EIO;

	int index = streamid - 1;
	uint32_t val = (8 | gray) << 4*index;
	bus_space_write_4(scp->bt, scp->bh, TS_GRAY_ADDR, val);
	return 0;
}
 
static int
ts_lock(struct ptx_softc *scp, uint8_t addr, uint16_t ts_id)
{
	int lp;
	WBLOCK wk;
	uint32_t val;
	union {
		uint8_t ts[2];
		uint16_t tsid;
	} uts_id;

	uts_id.tsid = ts_id;
	memcpy(&wk, &bs_set_ts_lock, sizeof(WBLOCK));
	wk.addr = addr;
	// TS-ID設定
	wk.value[1] = uts_id.ts[1];
	wk.value[2] = uts_id.ts[0];
	i2c_write(scp, &wk);

	for (lp = 0; lp < 100; lp++) {
		memcpy(&wk, &bs_get_ts_lock, sizeof(WBLOCK));
		wk.addr = addr;
		val = i2c_read(scp, &wk, 2);
		if ((val & 0xffff) == ts_id) {
			return 0;
		}
	}
	device_printf(scp->device, "ERROR TS-LOCK(%x)\n", ts_id);
	return EIO;
}

static int
bs_tune(struct ptx_softc *scp, uint8_t addr, int16_t channel, ISDB_S_TMCC *tmcc)
{
	int lp;
	int lp2;
	WBLOCK wk;
	uint32_t val;
	ISDB_S_TS_ID *tsid;
	union {
		uint8_t slot[4];
		uint32_t u32slot;
	} ts_slot;
	union {
		uint16_t ts[2];
		uint32_t tsid;
	} ts_id;

	if (channel >= MAX_BS_CHANNEL) {
		device_printf(scp->device, "Invalid Channel(%d)\n", channel);
		return EINVAL;
	}
	val = bs_frequency(scp, addr, channel);
	if (val == EIO) {
		return val;
	}

	tsid = &tmcc->ts_id[0];
	// 該当周波数のTS-IDを取得
	for (lp = 0; lp < (MAX_BS_TS_ID / 2); lp++) {
		for (lp2 = 0; lp2 < 100; lp2++) {
			memcpy(&wk, bs_get_ts_id[lp], sizeof(WBLOCK));
			wk.addr = addr;
			ts_id.tsid = i2c_read(scp, &wk, 4);
			// TS-IDが0の場合は再取得する
			if (ts_id.ts[0] != 0 && ts_id.ts[1] != 0) {
				break;
			}
		}
		tsid->ts_id = ts_id.ts[1];
		tsid += 1;
		tsid->ts_id = ts_id.ts[0];
		tsid += 1;
	}

	memcpy(&wk, &bs_get_agc, sizeof(WBLOCK));
	wk.addr = addr;
	tmcc->agc = i2c_read(scp, &wk, 1);

	// TS-ID別の情報を取得
	tsid = &tmcc->ts_id[0];
	for (lp = 0; lp < MAX_BS_TS_ID; lp++, tsid += 1) {
		// TS-IDなし=0XFFFF
		if (tsid->ts_id == 0xFFFF) {
			continue;
		}
		ts_lock(scp, addr, tsid->ts_id);

		//スロット取得
		memcpy(&wk, &bs_get_slot, sizeof(WBLOCK));
		wk.addr = addr;
		ts_slot.u32slot = i2c_read(scp, &wk, 3);
		tsid->high_mode = 0;
		tsid->low_slot  = ts_slot.slot[0];
		tsid->high_slot = ts_slot.slot[1];
		tsid->low_mode  = ts_slot.slot[2];
	}

	memcpy(&wk, &bs_get_clock, sizeof(WBLOCK));
	wk.addr = addr;
	tmcc->clockmargin = i2c_read(scp, &wk, 1);

	memcpy(&wk, &bs_get_carrir, sizeof(WBLOCK));
	wk.addr = addr;
	tmcc->carriermargin = i2c_read(scp, &wk, 1);
	return 0;
}

static int
bs_frequency(struct ptx_softc *scp, uint8_t addr, int16_t channel)
{
	int lp;
	int tmcclock = FALSE;
	WBLOCK wk;
	uint32_t val;

	if (channel >= MAX_BS_CHANNEL) {
		return EINVAL;
	}
	// ISDB-S PLLロック
	for (lp = 0; lp < MAX_BS_CHANNEL_PLL_COMMAND; lp++) {
		memcpy(&wk, bs_pll[channel].wblock[lp], sizeof(WBLOCK));
		wk.addr = addr;
		i2c_write(scp, &wk);
	}

	// PLLロック確認
	// チェック用
	for (lp = 0; lp < 200; lp++) {
		memcpy(&wk, &bs_pll_lock, sizeof(WBLOCK));
		wk.addr = addr;
		val = i2c_read(scp, &wk, 1);
		if ((val & 0xff) != 0 && (val & 0xff) != 0xff){
			tmcclock = TRUE;
			break;
		}
	}

	if (tmcclock == FALSE) {
		device_printf(scp->device, "PLL LOCK ERROR\n");
		return EIO;
	}

	memcpy(&wk, &bs_tmcc_get_1, sizeof(WBLOCK));
	wk.addr = addr;
	i2c_write(scp, &wk);

	tmcclock = FALSE;

	for (lp = 0; lp < 200; lp++) {
		memcpy(&wk, &bs_tmcc_get_2, sizeof(WBLOCK));
		wk.addr = addr;

		val = i2c_read(scp, &wk, 1);
		if ((val & 0xff) != 0xff && !(val & 0x10)) {
			tmcclock = TRUE;
			break;
		}
	}

	if (tmcclock == FALSE) {
		device_printf(scp->device, "TMCC LOCK ERROR\n");
		return EIO;
	}

	return 0;
}

static int
isdb_t_frequency(struct ptx_softc *scp, uint8_t addr, int16_t channel, int addfreq)
{
	int lp;
	WBLOCK wk;
	uint32_t val;
	int tmcclock = FALSE;
	union {
		uint8_t charfreq[2];
		uint16_t freq;
	} freq[2];

	if (channel >= MAX_ISDB_T_CHANNEL) {
		return EINVAL;
	}

	freq[0].freq = getfrequency(channel, addfreq);
	freq[1].freq = getfrequency_add(channel);
	//指定周波数
	memcpy(&wk, &isdb_t_pll_base, sizeof(WBLOCK));
	wk.addr = addr;
	// 計算した周波数を設定
	wk.value[wk.count] = freq[0].charfreq[1];
	wk.count += 1;
	wk.value[wk.count] = freq[0].charfreq[0];
	wk.count += 1;

	// 計算した周波数付加情報を設定
	wk.value[wk.count] = freq[1].charfreq[1];
	wk.count += 1;
	wk.value[wk.count] = freq[1].charfreq[0];
	wk.count += 1;

	i2c_write(scp, &wk);

	for (lp = 0; lp < 100; lp++) {
		memcpy(&wk, &isdb_t_pll_lock, sizeof(WBLOCK));
		wk.addr = addr;
		val = i2c_read(scp, &wk, 1);
		if ((val & 0xff) != 0xff && (val & 0x50) == 0x50) {
			tmcclock = TRUE;
			break;
		}
	}
	if (tmcclock != TRUE) {
		device_printf(scp->device, "ISDB-T LOCK NG(%08x)\n", val);
		return EIO;
	}

	memcpy(&wk, &isdb_t_check_tune, sizeof(WBLOCK));
	wk.addr = addr;
	i2c_write(scp, &wk);

	tmcclock = FALSE;
	for (lp = 0; lp < 1000; lp++) {
		memcpy(&wk, &isdb_t_tune_read, sizeof(WBLOCK));
		wk.addr = addr;
		val = i2c_read(scp, &wk, 1);
		if ((val & 0xff) != 0xff && (val & 0x8) != 8) {
			tmcclock = TRUE;
			break;
		}
	}
	if (tmcclock != TRUE) {
		device_printf(scp->device, "ISDB-T LOCK NG(%08x)\n", val);
		return EIO;
	}
	return 0;
}

static uint16_t
getfrequency(int16_t channel, uint16_t addfreq)
{
	uint16_t frequencyoffset = 0;
	uint16_t frequencyOffset = 0;

        if (12 <= channel)
                frequencyoffset += 2;
        if (17 <= channel)
                frequencyoffset -= 2;
        if (63 <= channel)
                frequencyoffset += 2;
#if 0
	return (((93 + channel * 6 + frequencyOffset) + addfreq) * 7) + 400;
#endif
	frequencyOffset = 93 + channel * 6 + frequencyoffset;
	frequencyOffset = 7 * (frequencyOffset + addfreq);
	return frequencyOffset + 400;
}

static uint16_t
getfrequency_add(int16_t channel)
{
	int lp;

	for (lp = 0; lp < 10; lp++) {
		if (channel <= isdb_t_freq_add[lp].pos) {
			return isdb_t_freq_add[lp].add_freq;
		}
	}
	return 0;
}

int
isdb_s_read_signal_strength(struct ptx_softc *scp, int streamid)
{
	uint8_t addr = geti2c(streamid);
	WBLOCK wk;
	uint32_t val;
	uint32_t val2;
	int val3;

	memcpy(&wk, &bs_get_signal1, sizeof(WBLOCK));
	wk.addr = addr;
	val = i2c_read(scp, &wk, 1);

	memcpy(&wk, &bs_get_signal2, sizeof(WBLOCK));
	wk.addr = addr;
	val2 = i2c_read(scp, &wk, 1);
	val3 = (((val << 8) & 0xff00) | (val2 & 0xff));

	return val3;
}

int
isdb_t_read_signal_strength(struct ptx_softc *scp, int streamid)
{
	uint8_t addr = geti2c(streamid);
	uint32_t val;
	uint32_t val2;
	uint32_t val3;
	WBLOCK wk;

	memcpy(&wk, &isdb_t_signal1, sizeof(WBLOCK));
	wk.addr = addr;
	val = i2c_read(scp, &wk, 1);

	memcpy(&wk, &isdb_t_signal2, sizeof(WBLOCK));
	wk.addr = addr;
	val2 = i2c_read(scp, &wk, 1);
	val3 = (((val << 8) & 0xff00) | (val2 & 0xff));
	return val3;
}

void
settuner_reset(struct ptx_softc *scp, uint32_t lnb, uint32_t tuner)
{
	uint32_t val = TUNER_POWER_OFF;
	switch (lnb) {
	case LNB_11V:
		val = (1 << BIT_LNB_DOWN);
		break;
	case LNB_15V:
		val = (1 << BIT_LNB_UP) | (1 << BIT_LNB_DOWN);
		break;
	}

	if (scp->cardtype == PT1) {
		switch (tuner) {
		case TUNER_POWER_ON_RESET_ENABLE:
			val |= (1 << BIT_TUNER);
			break;
		case TUNER_POWER_ON_RESET_DISABLE:
			val |= (1 << BIT_TUNER) | (1 << BIT_RESET);
			break;
		}
	} else if (scp->cardtype == PT2) {
		switch(tuner){
		case TUNER_POWER_ON_RESET_ENABLE:
			val |= (1 << BIT_TUNER)
			    | (1 << BIT_33A1)
			    | (1 << BIT_33A2)
			    | (1 << BIT_5A_)
			    | (1 << BIT_5A1)
			    | (1 << BIT_5A2);
			break;
		case TUNER_POWER_ON_RESET_DISABLE:
			val |= (1 << BIT_TUNER)
			    | (1 << BIT_RESET)
			    | (1 << BIT_33A1)
			    | (1 << BIT_33A2)
			    | (1 << BIT_5A_)
			    | (1 << BIT_5A1)
			    | (1 << BIT_5A2);
			break;
		}
	}

	bus_space_write_4(scp->bt, scp->bh, CFG_REGS_ADDR, val);
}

static uint8_t
geti2c(int streamid)
{
	static uint8_t i2c[] = {0, T0_ISDB_S, T0_ISDB_T, T1_ISDB_S, T1_ISDB_T};

	return i2c[streamid];
}

static int
getchtype(int streamid)
{
	if (streamid & 1)
		return CHANNEL_TYPE_ISDB_S;
	else
		return CHANNEL_TYPE_ISDB_T;
}
