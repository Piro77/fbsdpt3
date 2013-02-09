
#ifndef _PTX_H_
#define _PTX_H_

#include <sys/bus.h>
#include <sys/types.h>
#include <sys/mutex.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/condvar.h>

#include "pt3_misc.h"

// ------------------------------------------------------------------------

// pci
#define DMA_PAGE_SIZE 4096
#define DMA_BUFFER_PAGE_COUNT 511
#define DMA_VIRTUAL_COUNTXSIZE (4*16)

#define MAX_STREAM	4 // チャネル数

#define FALSE		0
#define TRUE		1
#define MAX_TUNER	2 //チューナ数

#define PACKET_SIZE	188 // 1パケット長
#define MAX_READ_BLOCK	4 // 1度に読み出す最大DMAバッファ数


//iic
#define FIFO_GO			0x04 // FIFO実行
#define FIFO_DONE		0x80 // FIFO 実行中ビット

#define FIFO_GO_ADDR		0x00 // FIFO 実行アドレス
#define FIFO_RESULT_ADDR	0x00 // FIFO 結果情報
#define CFG_REGS_ADDR		0x04
#define I2C_RESULT_ADDR		0x08 // I2C処理結果
#define FIFO_ADDR		0x10 // FIFOに書くアドレス
#define DMA_ADDR		0x14 // DMA設定に書くアドレス
#define TS_TEST_ENABLE_ADDR	(0x02 * 4) //
#define TS_GRAY_ADDR		(0x03 * 4)

// DMAエラー定義
#define MICROPACKET_ERROR	1 // Micro Packetエラー
#define BIT_RAM_OVERFLOW	(1 << 3)
#define BIT_INITIATOR_ERROR	(1 << 4)
#define BIT_INITIATOR_WARNING	(1 << 5)

#define PROGRAM_ADDRESS	1024

// I2Cデータ位置定義
#define I2C_ADDRESS	10 // I2Cアドレス(10ビット)

#define I2C_DATA_EN	10
#define I2C_CLOCK	11
#define I2C_WRITE_MODE	12 // I2C書き込み／読み込み
#define I2C_BUSY	13
#define I2C_DATA	18 // I2Cデータ(18ビット)

// I2C定義
#define WRITE_EN	1 // 書き込み
#define READ_EN		0 // 読み込み
#define DATA_EN		1 // データあり
#define DATA_DIS	0 // データなし
#define CLOCK_EN	1 // CLOCKあり
#define CLOCK_DIS	0 // CLOCKなし
#define BUSY_EN		1 // BUSYあり
#define BUSY_DIS	0 // BUSYなし

#define PCI_LOCKED	1
#define RAM_LOCKED	2
#define RAM_SHIFT	4

// ビット
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

// I2Cアドレス定義
#define T0_ISDB_S	0x1b // チューナ0 ISDB-S
#define T1_ISDB_S	0x19 // チューナ1 ISDB-S

#define T0_ISDB_T	0x1a // チューナ0 ISDB-T
#define T1_ISDB_T	0x18 // チューナ1 ISDB-T


//tuner
#define DATA_CHUNK_SIZE		(PACKET_SIZE * 16)
#define DATA_CHUNK_NUM		1024

// チューナ状態定義
#define MAX_BS_TS_ID		8 // TS-ID取得最大値
#define MAX_ISDB_T_INFO		3 // 地デジ階層情報数
#define MAX_ISDB_T_INFO_LEN	2 // 地デジ階層情報数



// ------------------------------------------------------------------------

enum {
	CHANNEL_TYPE_ISDB_S,
	CHANNEL_TYPE_ISDB_T,
	CHANNEL_TYPE_MAX
};

enum {
	STATE_STOP,		// 初期化直後
	STATE_START,		// 通常
	STATE_FULL		// ストッパー
};

enum {
	PT1 = 0,
	PT2,
	PT3,
};


// SLEEPモード設定
enum {
	TYPE_SLEEP,
	TYPE_WAKEUP
};

// チューナパワーモード設定
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

// LNBパワー設定
enum {
	LNB_OFF, // LNB OFF
	LNB_11V, // +11 V
	LNB_15V // +15 V
};

 // 電源／ハードウェアリセット
enum {
	TUNER_POWER_OFF, // オフ／イネーブル
	TUNER_POWER_ON_RESET_ENABLE, // オン／イネーブル
	TUNER_POWER_ON_RESET_DISABLE // オン／ディセーブル
};


// ------------------------------------------------------------------------

struct dmabuf {
	bus_dmamap_t map;
	uint32_t *pa;
	void *va;
};

struct ptx_stream {

	// |  1 | チューナー番号0 ISDB-S |
	// |  2 | チューナー番号0 ISDB-T |
	// |  3 | チューナー番号1 ISDB-S |
	// |  4 | チューナー番号1 ISDB-T |
	int id; // 1,2,3,4

	int freq;			// frequencyno|(slot<<16)

	int opened;
	int started;

	int drop;			// パケットドロップ数
	int overflow;			// オーバーフローエラー発生
	int counetererr;		// 転送カウンタ１エラー
	int transerr;			// 転送エラー


	/* ringbuf */
	uint8_t *buf;			// CH別受信メモリ

	int wp;
	int chunk_filled;

	int rp;
	int chunk_used;

	struct mtx lock;		// CH別mutex_lock用
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

	/* pt3 */
	bus_space_tag_t pt3_bt;
	bus_space_handle_t pt3_bh;
	struct resource *pt3_res_memory;
	int pt3_rid_memory;
	PT3_I2C *i2c;
	PT3_TUNER  tuner[MAX_TUNER];
	int pt3debug;
};

// I2C書き込みデータ定義
typedef	struct _WBLOCK {
	uint8_t	addr;	// I2Cデバイスアドレス
	uint32_t count;	// 転送個数
	uint8_t	value[16]; // 書き込み値
} WBLOCK;


// data
typedef struct _MICRO_PACKET {
	uint8_t data[3];
	uint8_t head;
} MICRO_PACKET;


// ISDB-S状態定義
typedef struct _ISDB_S_CH_TABLE {
	int channel;		// 入力チャンネル番号
	int real_chno;		// 実際のテーブル番号
	int slotno;		// スロット番号
} ISDB_S_CH_TABLE;

// ISDB-S状態定義
typedef	struct _ISDB_S_TS_ID {
	uint16_t ts_id;		// TS-ID
	uint16_t dmy;		// PAD
	uint8_t	low_mode;	// 低階層 モード
	uint8_t	low_slot;	// 低階層 スロット数
	uint8_t	high_mode;	// 高階層 モード
	uint8_t	high_slot;	// 高階層 スロット数
} ISDB_S_TS_ID;

typedef	struct _ISDB_S_TMCC {
	ISDB_S_TS_ID ts_id[MAX_BS_TS_ID];	// 相対TS番号nに対するTS ID情報
#if 0
	uint32_t indicator;			// 変更指示 (5ビット)
	uint32_t emergency;			// 起動制御信号 (1ビット)
	uint32_t uplink;			// アップリンク制御情報 (4ビット)
	uint32_t ext;				// 拡張フラグ (1ビット)
	uint32_t extdata[2];			// 拡張領域 (61ビット)
#endif
	uint32_t agc;				// AGC
	uint32_t clockmargin;			// クロック周波数誤差
	uint32_t carriermargin;			// キャリア周波数誤差
} ISDB_S_TMCC;

// 階層情報
typedef	struct _ISDB_T_INFO {
	uint32_t mode;				// キャリア変調方式 (3ビット)
	uint32_t rate;				// 畳込み符号化率 (3ビット)
	uint32_t interleave;			// インターリーブ長 (3ビット)
	uint32_t segment; 			// セグメント数 (4ビット)
} ISDB_T_INFO;

typedef	struct _ISDB_T_TMCC {
#if 0
	uint32_t sysid;		// システム識別 (2ビット)
	uint32_t indicator;	// 伝送パラメータ切り替え指標 (4ビット)
	uint32_t emergency;	// 緊急警報放送用起動フラグ (1ビット)
#endif
	ISDB_T_INFO info[MAX_ISDB_T_INFO];
#if 0
					// カレント情報
	uint32_t partial;		// 部分受信フラグ (1ビット)
	uint32_t Phase;			// 連結送信位相補正量 (3ビット)
	uint32_t Reserved;		// リザーブ (12ビット)
#endif
	uint32_t cn[2];			// CN
	uint32_t agc;			// AGC
	uint32_t clockmargin ;		// クロック周波数誤差
	uint32_t carriermargin ;	// キャリア周波数誤差
} ISDB_T_TMCC;


typedef struct _frequency {
	int frequencyno;	// 周波数テーブル番号
	int slot;		// スロット番号／加算する周波数
} FREQUENCY;

#define _MSTOTICK(t)	(hz * (t) / 1000)
#define MSTOTICK(t)	((t) == 0 ? 0 : (_MSTOTICK(t) == 0 ? 1 : _MSTOTICK(t)))

int ptx_pause(const char *wmesg, int timo);

#endif
