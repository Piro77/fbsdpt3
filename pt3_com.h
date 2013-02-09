/*******************************************************************************
   earthsoft PT3 Linux driver

   This program is free software; you can redistribute it and/or modify it
   under the terms and conditions of the GNU General Public License,
   version 3, as published by the Free Software Foundation.

   This program is distributed in the hope it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   The full GNU General Public License is included in this distribution in
   the file called "COPYING".

 *******************************************************************************/

#ifndef		__PT3_COM_H__
#define		__PT3_COM_H__

#define BIT_SHIFT_MASK(value, shift, mask) (((value) >> (shift)) & (((__u64)1<<(mask))-1))

#define		MAX_TUNER			2		//チューナ数
#define		MAX_CHANNEL			4		// チャネル数
#define		FALSE		0
#define		TRUE		1

enum {
	PT3_ISDB_S,
	PT3_ISDB_T,
	PT3_ISDB_MAX,
};

typedef enum {
	PT3_TS_PIN_MODE_NORMAL,
	PT3_TS_PIN_MODE_LOW,
	PT3_TS_PIN_MODE_HIGH,
} PT3_TS_PIN_MODE;

typedef struct _TS_PINS_MODE {
	int clock_data;
	int byte;
	int valid;
} PT3_TS_PINS_MODE;

typedef struct _TS_PINS_LEVEL {
	int clock;
	int data;
	int byte;
	int valid;
} PT3_TS_PINS_LEVEL;

enum LAYER_INDEX {
	LAYER_INDEX_L = 0,
	LAYER_INDEX_H,

	LAYER_INDEX_A = 0,
	LAYER_INDEX_B,
	LAYER_INDEX_C
};

enum LAYER_COUNT {
	LAYER_COUNT_S = LAYER_INDEX_H + 1,
	LAYER_COUNT_T = LAYER_INDEX_C + 1,
};

typedef struct __TMCC_S {
	__u32 indicator;
	__u32 mode[4];
	__u32 slot[4];
	__u32 id[8];
	__u32 emergency;
	__u32 uplink;
	__u32 extflag;
	__u32 extdata[2];
} TMCC_S;

typedef struct _TMCC_T {
	__u32 system;
	__u32 indicator;
	__u32 emergency;
	__u32 partial;
	__u32 mode[LAYER_COUNT_T];
	__u32 rate[LAYER_COUNT_T];
	__u32 interleave[LAYER_COUNT_T];
	__u32 segment[LAYER_COUNT_T];
	__u32 phase;
	__u32 reserved;
} TMCC_T;


typedef enum {
	// エラーなし
	STATUS_OK,

	// 一般的なエラー
	STATUS_GENERAL_ERROR = (1)*0x100,
	STATUS_NOT_IMPLIMENTED,
	STATUS_INVALID_PARAM_ERROR,
	STATUS_OUT_OF_MEMORY_ERROR,
	STATUS_INTERNAL_ERROR,

	// バスクラスのエラー
	STATUS_WDAPI_LOAD_ERROR = (2)*256,	// wdapi1100.dll がロードできない
	STATUS_ALL_DEVICES_MUST_BE_DELETED_ERROR,

	// デバイスクラスのエラー
	STATUS_PCI_BUS_ERROR = (3)*0x100,
	STATUS_CONFIG_REVISION_ERROR,
	STATUS_FPGA_VERSION_ERROR,
	STATUS_PCI_BASE_ADDRESS_ERROR,
	STATUS_FLASH_MEMORY_ERROR,

	STATUS_DCM_LOCK_TIMEOUT_ERROR,
	STATUS_DCM_SHIFT_TIMEOUT_ERROR,

	STATUS_POWER_RESET_ERROR,
	STATUS_I2C_ERROR,
	STATUS_TUNER_IS_SLEEP_ERROR,

	STATUS_PLL_OUT_OF_RANGE_ERROR,
	STATUS_PLL_LOCK_TIMEOUT_ERROR,

	STATUS_VIRTUAL_ALLOC_ERROR,
	STATUS_DMA_ADDRESS_ERROR,
	STATUS_BUFFER_ALREADY_ALLOCATED_ERROR,

	STATUS_DEVICE_IS_ALREADY_OPEN_ERROR,
	STATUS_DEVICE_IS_NOT_OPEN_ERROR,

	STATUS_BUFFER_IS_IN_USE_ERROR,
	STATUS_BUFFER_IS_NOT_ALLOCATED_ERROR,

	STATUS_DEVICE_MUST_BE_CLOSED_ERROR,

	// WinDriver 関連のエラー
	STATUS_WD_DriverName_ERROR = (4)*0x100,

	STATUS_WD_Open_ERROR,
	STATUS_WD_Close_ERROR,

	STATUS_WD_Version_ERROR,
	STATUS_WD_License_ERROR,

	STATUS_WD_PciScanCards_ERROR,

	STATUS_WD_PciConfigDump_ERROR,

	STATUS_WD_PciGetCardInfo_ERROR,
	STATUS_WD_PciGetCardInfo_Bus_ERROR,
	STATUS_WD_PciGetCardInfo_Memory_ERROR,

	STATUS_WD_CardRegister_ERROR,
	STATUS_WD_CardUnregister_ERROR,

	STATUS_WD_CardCleanupSetup_ERROR,

	STATUS_WD_DMALock_ERROR,
	STATUS_WD_DMAUnlock_ERROR,

	STATUS_WD_DMASyncCpu_ERROR,
	STATUS_WD_DMASyncIo_ERROR,

	// ROM
	STATUS_ROM_ERROR = (5)*0x100,
	STATUS_ROM_TIMEOUT
} STATUS;

#endif
