#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include "ptx.h"
#include "ptx_tuner.h"

#include "ptx_iic.h"

static void
makei2c(struct ptx_softc *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
static int
i2c_lock(struct ptx_softc *, uint32_t, uint32_t, uint32_t);
static int
i2c_lock_one(struct ptx_softc *, uint32_t, uint32_t);
static int
i2c_unlock(struct ptx_softc *, int);
static void
blockwrite(struct ptx_softc *, WBLOCK *);
static void
blockread(struct ptx_softc *, WBLOCK *, int);
static void
writebits(struct ptx_softc *, uint32_t *, uint32_t, uint32_t);
static void
begin_i2c(struct ptx_softc *, uint32_t *, uint32_t *);
static void
start_i2c(struct ptx_softc *, uint32_t *, uint32_t *, uint32_t);
static void
stop_i2c(struct ptx_softc *, uint32_t *, uint32_t *, uint32_t, uint32_t);

// PCIに書き込むI2Cデータ生成
static void
makei2c(struct ptx_softc *scp, uint32_t base_addr, uint32_t i2caddr, uint32_t writemode, uint32_t data_en, uint32_t clock, uint32_t busy)
{
	uint32_t val;
	val =  ((base_addr << I2C_DATA)
	    | (writemode << I2C_WRITE_MODE)
	    | (data_en << I2C_DATA_EN)
	    | (clock << I2C_CLOCK)
	    | (busy << I2C_BUSY)
	    | i2caddr);
	bus_space_write_4(scp->bt, scp->bh, FIFO_ADDR, val);
}

int
xc3s_init(struct ptx_softc *scp)
{
	uint32_t val;
	int lp;
	int rc;
	int phase = XC3S_PCI_CLOCK;

/*
  val = (1 << 19) | (1 << 27) | (1 << 16) | (1 << 24) | (1 << 17) | (1 << 25);
  writel(WRITE_PULSE, regs);
  BIT 19, 19+8 ON
  BIT 16, 16+8 ON
  BIT 17, 17+8 ON
*/
	// XC3S初期化
	for (lp = 0; lp < PROGRAM_ADDRESS; lp++) {
		makei2c(scp, lp, 0, READ_EN, DATA_DIS, CLOCK_DIS, BUSY_DIS);
	}
	// XC3S 初期化待ち (512 PCI Clocks)
	for (lp = 0; lp < XC3S_PCI_CLOCK; lp++) {
		makei2c(scp, 0, 0, READ_EN, DATA_DIS, CLOCK_DIS, BUSY_DIS);
	}
	// プロテクト解除
	// これは何を意図しているんだろう？
	// 元コードが良く判らない
	for (lp = 0; lp < 57; lp++) {
		val = bus_space_read_4(scp->bt, scp->bh, 0x0);
		if (val & I2C_READ_SYNC) {
			break;
		}
		bus_space_write_4(scp->bt, scp->bh, 0x0, WRITE_PULSE);
	}

	for (lp = 0; lp < 57; lp++) {
		val = bus_space_read_4(scp->bt, scp->bh, 0x0);
		bus_space_write_4(scp->bt, scp->bh, 0x0, WRITE_PULSE);
	}
	bus_space_write_4(scp->bt, scp->bh, 0x0, WRITE_PULSE);

	// UNLOCK
	rc = i2c_unlock(scp, READ_UNLOCK);
	if (rc) {
		return rc;
	}

	// Enable PCI
	rc =i2c_lock(scp, (WRITE_PCI_RESET | WRITE_PCI_RESET_),
	    WRITE_PCI_RESET_, PCI_LOCKED);
	if (rc) {
		return rc;
	}

	// Enable RAM
	rc = i2c_lock(scp, (WRITE_RAM_RESET | WRITE_RAM_RESET_),
	    WRITE_RAM_RESET_, RAM_LOCKED);
	if (rc) {
		return rc;
	}

	ptx_pause("ptxenram", MSTOTICK(1000));

	switch(scp->cardtype) {
        case PT1:
		phase = XC3S_PCI_CLOCK;
		break;
	case PT2:
		phase = XC3S_PCI_CLOCK_PT2;
		break;
	}
	for (lp = 0; lp < phase; lp++) {
		rc = i2c_lock_one(scp, WRITE_RAM_ENABLE, RAM_SHIFT);
		if (rc) {
			device_printf(scp->device, "LOCK FAULT\n");
			return rc;
		}
	}

	// ストリームごとの転送制御(OFF)
	for (lp = 1; lp <= MAX_STREAM; lp++) {
		if ((rc = SetStream(scp, lp, FALSE)) ||
		    (rc = SetStreamGray(scp, lp, 0)))
			return rc;
	}
	return 0;
}

static int
i2c_lock(struct ptx_softc *scp, uint32_t firstval, uint32_t  secondval, uint32_t lockval)
{
	uint32_t val;
	int lp;

	bus_space_write_4(scp->bt, scp->bh, 0x0, firstval);
	bus_space_write_4(scp->bt, scp->bh, 0x0, secondval);

	// RAMがロックされた？
	for (lp = 0; lp < 10; lp++) {
		val = bus_space_read_4(scp->bt, scp->bh, 0x0);
		if (val & lockval) {
			return 0;
		}
		ptx_pause("ptxi2l", MSTOTICK(1));
	}
	return EIO;
}

static int
i2c_lock_one(struct ptx_softc *scp, uint32_t firstval, uint32_t lockval)
{
	uint32_t val;
	uint32_t val2;
	int lp, lp2;

	val = (bus_space_read_4(scp->bt, scp->bh, 0x0) & lockval);

	bus_space_write_4(scp->bt, scp->bh, 0x0, firstval);

	// RAMがロックされた？
	for (lp = 0; lp < 10; lp++) { // ???
		for (lp2 = 0; lp2 < 1024; lp2++){
			val2 = bus_space_read_4(scp->bt, scp->bh, 0x0);
			// 最初に取得したデータと逆になればOK
			if ((val2 & lockval) != val) {
				return 0;
			}
		}
		ptx_pause("ptxi2l", MSTOTICK(1000));
	}
	device_printf(scp->device, "Lock Fault(%x:%x)\n", val, val2);
	return EIO;
}

static int
i2c_unlock(struct ptx_softc *scp, int lockval)
{
	int lp;
	uint32_t val;

	for (lp = 0; lp < 3; lp++) {
		val = bus_space_read_4(scp->bt, scp->bh, 0x0);
		if (val &lockval) {
			return 0;
		}
		ptx_pause("ptxi2u", MSTOTICK(1000));
	}
	return EIO;
}

static void
blockwrite(struct ptx_softc *scp, WBLOCK *wblock)
{
	int lp;
	int bitpos;
	uint32_t bits;
	uint32_t old_bits = 1;
	uint32_t address = 0;
	uint32_t clock = 0;

	begin_i2c(scp, &address, &clock);
	if (scp->i2c_state == STATE_STOP) {
		start_i2c(scp, &address, &clock, old_bits);
		old_bits = 0;
		stop_i2c(scp, &address, &clock, old_bits, FALSE);
		scp->i2c_state = STATE_START;
	}
	old_bits = 1;
	start_i2c(scp, &address, &clock, old_bits);
	old_bits = 0;

	// まずI2Cスレーブアドレスを書く(7bit)
	for (bitpos = 0; bitpos < 7; bitpos++) {
		bits  = ((wblock->addr >> (6 - bitpos)) & 1);
		writebits(scp, &address, old_bits, bits);
		old_bits = bits;
	}
	// タイプ：WRT
	writebits(scp, &address, old_bits, 0);
	// ACK/NACK用(必ず1)
	writebits(scp, &address, 0, 1);

	old_bits = 1;
	// I2Cスレーブデバイスにデータを書く
	for (lp = 0; lp < wblock->count; lp++) {
		for (bitpos = 0; bitpos < 8; bitpos++) {
			bits = ((wblock->value[lp] >> (7 - bitpos)) & 1);
			writebits(scp, &address, old_bits, bits);
			old_bits = bits;
		}
		// ACK/NACK用(必ず1)
		writebits(scp, &address, old_bits, 1);
		old_bits = 1;
	}

	// Clock negedge
	makei2c(scp, address, address + 1, 0, (old_bits ^ 1), 1, 1);
	clock = TRUE;
	address += 1;
	stop_i2c(scp, &address, &clock, old_bits, TRUE);
}

static void
blockread(struct ptx_softc *scp, WBLOCK *wblock, int count)
{
	int lp;
	int bitpos;
	uint32_t bits;
	uint32_t old_bits = 1;
	uint32_t address = 0;
	uint32_t clock = 0;

	begin_i2c(scp, &address, &clock);
	if (scp->i2c_state == STATE_STOP) {
		start_i2c(scp, &address, &clock, old_bits);
		old_bits = 0;
		stop_i2c(scp, &address, &clock, old_bits, FALSE);
		scp->i2c_state = STATE_START;
	}
	old_bits = 1;
	start_i2c(scp, &address, &clock, old_bits);
	old_bits = 0;

	// まずI2Cスレーブアドレスを書く(7bit)
	for (bitpos = 0; bitpos < 7; bitpos++) {
		bits = ((wblock->addr >> (6 - bitpos)) & 1);
		writebits(scp, &address, old_bits, bits);
		old_bits = bits;
	}
	// タイプ：WRT
	writebits(scp, &address, old_bits, 0);
	// ACK/NACK用(必ず1)
	writebits(scp, &address, 0, 1);

	old_bits = 1;
	// I2Cスレーブデバイスにデータを書く(Readしたいアドレス)
	for (lp = 0; lp < wblock->count; lp++) {
		for (bitpos = 0; bitpos < 8; bitpos++) {
			bits = ((wblock->value[lp] >> (7 - bitpos)) & 1);
			writebits(scp, &address, old_bits, bits);
			old_bits = bits;
		}
		// ACK/NACK用(必ず1)
		writebits(scp, &address, old_bits, 1);
		old_bits = 1;
	}

	// Clock negedge
	makei2c(scp, address, address + 1, 0, (old_bits ^ 1), 1, 1);
	clock = TRUE;
	address += 1;

	// ここから Read
	start_i2c(scp, &address, &clock, old_bits);
	old_bits = 0;
	// もう一度I2Cスレーブアドレスを書く(7bit)
	for (bitpos = 0; bitpos < 7; bitpos++) {
		bits = ((wblock->addr >> (6 - bitpos)) & 1);
		writebits(scp, &address, old_bits, bits);
		old_bits = bits;
	}
	// タイプ：RD
	writebits(scp, &address, old_bits, 1);
	// ACK/NACK用(必ず1)
	writebits(scp, &address, 1, 1);

	old_bits = 1;
	// I2CスレーブデバイスからデータをReadする
	for (lp = 0; lp < count; lp++) {
		for (bitpos = 0; bitpos < 8; bitpos++) {
			// データをReadするため常に1を出力する
			// クロックラインを動かすためにwritebitsを使う
			writebits(scp, &address, old_bits, 1);
			// Read Mode Set
			// 多分これでFPGAのFIFOに入れる
			makei2c(scp, address, address + 1, 1, 0, 0, 1);
			address += 1;
			old_bits = 1;
		}
		if (lp >= (count - 1)) {
			// ACK/NACK用(必ず1)
			writebits(scp, &address, old_bits, 1);
			old_bits = 0;
		} else {
			// ACK/NACK用(必ず1)
			writebits(scp, &address, old_bits, 0);
			old_bits = 1;
		}
	}

	// Clock negedge
	makei2c(scp, address, address + 1, 0, 0, 1, 1);
	clock = TRUE;
	address += 1;
	old_bits = 1;
	stop_i2c(scp, &address, &clock, old_bits, TRUE);
}

static void
writebits(struct ptx_softc *scp, uint32_t *address, uint32_t old_bits, uint32_t bits)
{
	// CLOCK UP
	makei2c(scp, *address, *address + 1, 0, (old_bits ^ 1), 1, 1);
	*address += 1;

	// CLOCK UP
	makei2c(scp, *address, *address + 1, 0, (bits ^ 1), 1, 1);
	*address += 1;

	// CLOCK DOWN
	makei2c(scp, *address, *address + 1, 0, (bits ^ 1), 0, 1);
	*address += 1;
}

static void
begin_i2c(struct ptx_softc *scp, uint32_t *address, uint32_t *clock)
{
	// bus FREE
	makei2c(scp, *address, *address, 0, 0, 0, 0);
	*address += 1;

	//  bus busy
	makei2c(scp, *address, *address + 1, 0, 0, 0, 1);
	*address += 1;
	*clock = FALSE;
}

static void
start_i2c(struct ptx_softc *scp, uint32_t *address, uint32_t *clock, uint32_t data)
{
	// データが残っていなければデータを下げる
	if (!data) {
		// CLOCKがあればCLOCKを下げる
		if (*clock != TRUE) {
			*clock = TRUE;
			makei2c(scp, *address, *address + 1, 0, 1, 1, 1);
			*address += 1;
		}
		makei2c(scp, *address, *address + 1, 0, 0, 1, 1);
		*address += 1;
	}

	if (*clock != FALSE) {
		*clock = FALSE;
		makei2c(scp, *address, *address + 1, 0, 0, 0, 1);
		*address += 1;
	}
	makei2c(scp, *address, *address + 1, 0, 1, 0, 1);
	*address += 1;
	*clock = FALSE;
}

static void
stop_i2c(struct ptx_softc *scp, uint32_t *address, uint32_t *clock, uint32_t data, uint32_t end)
{
	// データが残っていて
	if (data) {
		// クロックがあれば
		if (*clock != TRUE) {
			*clock = TRUE;
			makei2c(scp, *address, *address + 1, 0, 0, 1, 1);
			*address += 1;
		}
		makei2c(scp, *address, *address + 1, 0, 1, 1, 1);
		*address += 1;
	}
	// クロックが落ちていれば
	if (*clock) {
		*clock = FALSE;
		makei2c(scp, *address, *address + 1, 0, 1, 0, 1);
		*address += 1;
	}

	if (end) {
		makei2c(scp, *address, 0, 0, 0, 0, 1);
	} else {
		makei2c(scp, *address, *address + 1, 0, 0, 0, 1);
		*address += 1;
	}
}

void
i2c_write(struct ptx_softc *scp, WBLOCK *wblock)
{
	int lp;
	uint32_t val;

	mtx_lock(&scp->lock);
	while (scp->i2c_progress) {
		msleep(&scp->i2c_progress, &scp->lock, 0|PCATCH, "ptxi2w", 0);
	}
	scp->i2c_progress = 1;
	mtx_unlock(&scp->lock);

	blockwrite(scp, wblock);

	bus_space_write_4(scp->bt, scp->bh, FIFO_GO_ADDR, FIFO_GO);
	//とりあえずロックしないように。
	for (lp = 0; lp < 100; lp++) {
		val = bus_space_read_4(scp->bt, scp->bh, FIFO_RESULT_ADDR);
		if (!(val & FIFO_DONE)) {
			break;
		}
		ptx_pause("ptxi2w", MSTOTICK(1));
	}
	if (lp >= 100)
		device_printf(scp->device, "bus_space_read_4 timeoout\n");

	mtx_lock(&scp->lock);
	scp->i2c_progress = 0;
	wakeup(&scp->i2c_progress);
	mtx_unlock(&scp->lock);
}

uint32_t
i2c_read(struct ptx_softc *scp, WBLOCK *wblock, int size)
{
	int lp;
	uint32_t val;

	mtx_lock(&scp->lock);
	while (scp->i2c_progress) {
		msleep(&scp->i2c_progress, &scp->lock, 0|PCATCH, "ptxi2w", 0);
	}
	scp->i2c_progress = 1;
	mtx_unlock(&scp->lock);

	blockread(scp, wblock, size);

	bus_space_write_4(scp->bt, scp->bh, FIFO_GO_ADDR, FIFO_GO);

	for (lp = 0; lp < 100; lp++) {
		ptx_pause("ptxi2r", MSTOTICK(1));

		val = bus_space_read_4(scp->bt, scp->bh, FIFO_RESULT_ADDR);
		if (!(val & FIFO_DONE)) {
			break;
		}
	}
	if (lp >= 100)
		device_printf(scp->device, "bus_space_read_4 timeoout\n");
	val = bus_space_read_4(scp->bt, scp->bh, I2C_RESULT_ADDR);

	mtx_lock(&scp->lock);
	scp->i2c_progress = 0;
	wakeup(&scp->i2c_progress);
	mtx_unlock(&scp->lock);

	return val;
}
