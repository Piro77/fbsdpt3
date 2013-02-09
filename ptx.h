
#ifndef _PTX_H_
#define _PTX_H_

#include <sys/bus.h>
#include <sys/types.h>
#include <sys/mutex.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/condvar.h>

// ------------------------------------------------------------------------

// pci
#define DMA_PAGE_SIZE 4096
#define DMA_BUFFER_PAGE_COUNT 511
#define DMA_VIRTUAL_COUNTXSIZE (4*16)

#define MAX_STREAM	4 // ����ͥ��

#define FALSE		0
#define TRUE		1
#define MAX_TUNER	2 //���塼�ʿ�

#define PACKET_SIZE	188 // 1�ѥ��å�Ĺ
#define MAX_READ_BLOCK	4 // 1�٤��ɤ߽Ф�����DMA�Хåե���


//iic
#define FIFO_GO			0x04 // FIFO�¹�
#define FIFO_DONE		0x80 // FIFO �¹���ӥå�

#define FIFO_GO_ADDR		0x00 // FIFO �¹ԥ��ɥ쥹
#define FIFO_RESULT_ADDR	0x00 // FIFO ��̾���
#define CFG_REGS_ADDR		0x04
#define I2C_RESULT_ADDR		0x08 // I2C�������
#define FIFO_ADDR		0x10 // FIFO�˽񤯥��ɥ쥹
#define DMA_ADDR		0x14 // DMA����˽񤯥��ɥ쥹
#define TS_TEST_ENABLE_ADDR	(0x02 * 4) //
#define TS_GRAY_ADDR		(0x03 * 4)

// DMA���顼���
#define MICROPACKET_ERROR	1 // Micro Packet���顼
#define BIT_RAM_OVERFLOW	(1 << 3)
#define BIT_INITIATOR_ERROR	(1 << 4)
#define BIT_INITIATOR_WARNING	(1 << 5)

#define PROGRAM_ADDRESS	1024

// I2C�ǡ����������
#define I2C_ADDRESS	10 // I2C���ɥ쥹(10�ӥå�)

#define I2C_DATA_EN	10
#define I2C_CLOCK	11
#define I2C_WRITE_MODE	12 // I2C�񤭹��ߡ��ɤ߹���
#define I2C_BUSY	13
#define I2C_DATA	18 // I2C�ǡ���(18�ӥå�)

// I2C���
#define WRITE_EN	1 // �񤭹���
#define READ_EN		0 // �ɤ߹���
#define DATA_EN		1 // �ǡ�������
#define DATA_DIS	0 // �ǡ����ʤ�
#define CLOCK_EN	1 // CLOCK����
#define CLOCK_DIS	0 // CLOCK�ʤ�
#define BUSY_EN		1 // BUSY����
#define BUSY_DIS	0 // BUSY�ʤ�

#define PCI_LOCKED	1
#define RAM_LOCKED	2
#define RAM_SHIFT	4

// �ӥå�
#define WRITE_PCI_RESET		(1 << 16)
#define WRITE_PCI_RESET_	(1 << 24)
#define WRITE_RAM_RESET		(1 << 17)
#define WRITE_RAM_RESET_	(1 << 25)
#define WRITE_RAM_ENABLE	(1 << 1)

#define WRITE_PULSE		(1 << 3)
#define I2C_READ_SYNC		(1 << 29)
#define READ_DATA		(1 << 30)
#define READ_UNLOCK		(1 << 31)

#define XC3S_PCI_CLOCK		(512 / 4)
#define XC3S_PCI_CLOCK_PT2	(166)

// I2C���ɥ쥹���
#define T0_ISDB_S	0x1b // ���塼��0 ISDB-S
#define T1_ISDB_S	0x19 // ���塼��1 ISDB-S

#define T0_ISDB_T	0x1a // ���塼��0 ISDB-T
#define T1_ISDB_T	0x18 // ���塼��1 ISDB-T


//tuner
#define DATA_CHUNK_SIZE		(PACKET_SIZE * 16)
#define DATA_CHUNK_NUM		1024

// ���塼�ʾ������
#define MAX_BS_TS_ID		8 // TS-ID����������
#define MAX_ISDB_T_INFO		3 // �ϥǥ����ؾ����
#define MAX_ISDB_T_INFO_LEN	2 // �ϥǥ����ؾ����



// ------------------------------------------------------------------------

enum {
	CHANNEL_TYPE_ISDB_S,
	CHANNEL_TYPE_ISDB_T,
	CHANNEL_TYPE_MAX
};

enum {
	STATE_STOP,		// �����ľ��
	STATE_START,		// �̾�
	STATE_FULL		// ���ȥåѡ�
};

enum {
	PT1 = 0,
	PT2,
};


// SLEEP�⡼������
enum {
	TYPE_SLEEP,
	TYPE_WAKEUP
};

// ���塼�ʥѥ�⡼������
enum {
	BIT_TUNER,
	BIT_LNB_UP,
	BIT_LNB_DOWN,
	BIT_RESET,
	BIT_33A1,
	BIT_33A2,
	BIT_5A_,
	BIT_5A1,
	BIT_5A2
};

// LNB�ѥ����
enum {
	LNB_OFF, // LNB OFF
	LNB_11V, // +11 V
	LNB_15V // +15 V
};

 // �Ÿ����ϡ��ɥ������ꥻ�å�
enum {
	TUNER_POWER_OFF, // ���ա����͡��֥�
	TUNER_POWER_ON_RESET_ENABLE, // ���󡿥��͡��֥�
	TUNER_POWER_ON_RESET_DISABLE // ���󡿥ǥ������֥�
};


// ------------------------------------------------------------------------

struct dmabuf {
	bus_dmamap_t map;
	uint32_t *pa;
	void *va;
};

struct ptx_stream {

	// |  1 | ���塼�ʡ��ֹ�0 ISDB-S |
	// |  2 | ���塼�ʡ��ֹ�0 ISDB-T |
	// |  3 | ���塼�ʡ��ֹ�1 ISDB-S |
	// |  4 | ���塼�ʡ��ֹ�1 ISDB-T |
	int id; // 1,2,3,4

	int freq;			// frequencyno|(slot<<16)

	int opened;
	int started;

	int drop;			// �ѥ��åȥɥ�å׿�
	int overflow;			// �����С��ե����顼ȯ��
	int counetererr;		// ž�������󥿣����顼
	int transerr;			// ž�����顼


	/* ringbuf */
	uint8_t *buf;			// CH�̼�������

	int wp;
	int chunk_filled;

	int rp;
	int chunk_used;

	struct mtx lock;		// CH��mutex_lock��
	struct cv not_full;
	struct cv not_empty;
};

struct ptx_softc {
	device_t device;
	int unit;
	int cardtype;

	struct mtx lock;

	struct dmabuf ringbuf[DMA_VIRTUAL_COUNTXSIZE];
	struct dmabuf databuf[DMA_VIRTUAL_COUNTXSIZE][DMA_BUFFER_PAGE_COUNT];

	bus_dma_tag_t dmat;
	bus_space_tag_t bt;
	bus_space_handle_t bh;
	int rid_memory;
	struct resource *res_memory;

	struct cdev *dev[MAX_STREAM];
	struct ptx_stream stream[MAX_STREAM];

#if (__FreeBSD_version < 800000)
	struct proc *ptxdaemon;
#else
	struct thread *ptxdaemon;
#endif
	int ring_pos;
	int data_pos;

	int procrun;

	int lnb; /* LNB OFF:0 +11V:1 +15V:2 */

	int i2c_state;
	int i2c_progress;
};

// I2C�񤭹��ߥǡ������
typedef	struct _WBLOCK {
	uint8_t	addr;	// I2C�ǥХ������ɥ쥹
	uint32_t count;	// ž���Ŀ�
	uint8_t	value[16]; // �񤭹�����
} WBLOCK;


// data
typedef struct _MICRO_PACKET {
	uint8_t data[3];
	uint8_t head;
} MICRO_PACKET;


// ISDB-S�������
typedef struct _ISDB_S_CH_TABLE {
	int channel;		// ���ϥ����ͥ��ֹ�
	int real_chno;		// �ºݤΥơ��֥��ֹ�
	int slotno;		// ����å��ֹ�
} ISDB_S_CH_TABLE;

// ISDB-S�������
typedef	struct _ISDB_S_TS_ID {
	uint16_t ts_id;		// TS-ID
	uint16_t dmy;		// PAD
	uint8_t	low_mode;	// �㳬�� �⡼��
	uint8_t	low_slot;	// �㳬�� ����åȿ�
	uint8_t	high_mode;	// �ⳬ�� �⡼��
	uint8_t	high_slot;	// �ⳬ�� ����åȿ�
} ISDB_S_TS_ID;

typedef	struct _ISDB_S_TMCC {
	ISDB_S_TS_ID ts_id[MAX_BS_TS_ID];	// ����TS�ֹ�n���Ф���TS ID����
#if 0
	uint32_t indicator;			// �ѹ��ؼ� (5�ӥå�)
	uint32_t emergency;			// ��ư���濮�� (1�ӥå�)
	uint32_t uplink;			// ���åץ��������� (4�ӥå�)
	uint32_t ext;				// ��ĥ�ե饰 (1�ӥå�)
	uint32_t extdata[2];			// ��ĥ�ΰ� (61�ӥå�)
#endif
	uint32_t agc;				// AGC
	uint32_t clockmargin;			// ����å����ȿ���
	uint32_t carriermargin;			// ����ꥢ���ȿ���
} ISDB_S_TMCC;

// ���ؾ���
typedef	struct _ISDB_T_INFO {
	uint32_t mode;				// ����ꥢ��Ĵ���� (3�ӥå�)
	uint32_t rate;				// ��������沽Ψ (3�ӥå�)
	uint32_t interleave;			// ���󥿡��꡼��Ĺ (3�ӥå�)
	uint32_t segment; 			// �������ȿ� (4�ӥå�)
} ISDB_T_INFO;

typedef	struct _ISDB_T_TMCC {
#if 0
	uint32_t sysid;		// �����ƥ༱�� (2�ӥå�)
	uint32_t indicator;	// �����ѥ�᡼���ڤ��ؤ���ɸ (4�ӥå�)
	uint32_t emergency;	// �۵޷��������ѵ�ư�ե饰 (1�ӥå�)
#endif
	ISDB_T_INFO info[MAX_ISDB_T_INFO];
#if 0
					// �����Ⱦ���
	uint32_t partial;		// ��ʬ�����ե饰 (1�ӥå�)
	uint32_t Phase;			// Ϣ���������������� (3�ӥå�)
	uint32_t Reserved;		// �ꥶ���� (12�ӥå�)
#endif
	uint32_t cn[2];			// CN
	uint32_t agc;			// AGC
	uint32_t clockmargin ;		// ����å����ȿ���
	uint32_t carriermargin ;	// ����ꥢ���ȿ���
} ISDB_T_TMCC;


typedef struct _frequency {
	int frequencyno;	// ���ȿ��ơ��֥��ֹ�
	int slot;		// ����å��ֹ桿�û�������ȿ�
} FREQUENCY;

#define _MSTOTICK(t)	(hz * (t) / 1000)
#define MSTOTICK(t)	((t) == 0 ? 0 : (_MSTOTICK(t) == 0 ? 1 : _MSTOTICK(t)))

int ptx_pause(const char *wmesg, int timo);
#endif
