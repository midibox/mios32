// $Id: mios32_can.h 2403 2016-08-15 17:47:50Z tk $
/*
 * Header file for CAN functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

#ifndef _MIOS32_CAN_H
#define _MIOS32_CAN_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of CAN interfaces (0..2)
#if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#ifndef MIOS32_CAN_NUM
#define MIOS32_CAN_NUM 1
#else
#if MIOS32_CAN_NUM >1
#define MIOS32_CAN_NUM 1
#endif
#endif
#else
#define MIOS32_CAN_NUM 0
# warning "Unsupported MIOS32_BOARD selected!"
// because of MIDI Area Network Id arbitration and filtering, only STM32F4 is supported.
#endif

// Tx buffer size (1..256)
#ifndef MIOS32_CAN_TX_BUFFER_SIZE
#define MIOS32_CAN_TX_BUFFER_SIZE 128
// Should be enough for the 1024 bytes of Sysex buffer
// Note: One CAN packet transmits 8 bytes maximum.
// We need 1024/8 = 128 packet buffer.
#endif

// Rx buffer size (1..256)
#ifndef MIOS32_CAN_RX_BUFFER_SIZE
#define MIOS32_CAN_RX_BUFFER_SIZE 128
#endif

// Interface assignment: 0 = disabled, 1 = MIDI, (2 = MBNET ?), (3 = other ?)
#ifndef MIOS32_CAN1_ASSIGNMENT
#define MIOS32_CAN1_ASSIGNMENT    1
#endif

// Interface assignment: 0 = disabled, 1 = MIDI, (2 = MBNET ?), (3 = other ?)
#ifndef MIOS32_CAN2_ASSIGNMENT
#define MIOS32_CAN2_ASSIGNMENT    0
#endif

// Alternate function pin assignement.
//  0: CAN2.RX->PB5, CAN2.TX->PB6
//  1: CAN2.RX->PB12, CAN2.TX->PB13
#ifndef MIOS32_CAN2_ALTFUNC
#define MIOS32_CAN2_ALTFUNC       0
#endif

#define MIOS32_CAN_VERBOSE        1



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// CAN Standard 11bits Id(16bis reg), used for 16bits filtering only!
typedef union {
  u16 ALL;
  struct {
    u16 :3;
    u16 ide:1;
    u16 rtr:1;
    u16 :11;
  };
  
} mios32_can_std_id_t;

// CAN Extended 29bits Id(32Bits reg), CAN_RIxR/CAN_TIxR
typedef union {
  u32 ALL;
  struct {
    u16 data[2];
  };
  struct {
    u16 data_l;
    u16 data_h;
  };
  struct {
    u32 TXRQ:1;
    u32 RTR:1;
    u32 IDE:1;
    u32 :29;
  };
  struct {
    u32 txrq:1;
    u32 rtr:1;
    u32 ide:1;
    u32 :29;
  };
  struct {
    u32 :21;
    u32 std_id:11;
  };
  struct {
    u32 :1;
    u32 lpc_ctrl:2; // special field for lpc,
    u32 ext_id:29;
  };
} mios32_can_ext_id_t;

// CAN control registers CAN_TDTxR/CAN_RDTxR
typedef union {
  u32 ALL;
  struct {
    u32 dlc:4;
    u32 :2;
    u32 fmi:10;
    u32 time:16;
  };
  struct {
    u32 DLC:4;
    u32 :4;
    u32 tgt:1;
    u32 :23;
  };
} mios32_can_ctrl_t;

// CAN data registers CAN_TDLxR/CAN_TDHxR/CAN_RDLxR/CAN_RDHxR
typedef union {
  struct {
    u32 ALL[2];
  };
  struct {
    u32 data_l;
    u32 data_h;
  };
  struct {
    u16 data[4];
  };
  struct {
    u8 bytes[8];
  };
} mios32_can_data_t;

// CAN mailboxes packet
typedef struct mios32_can_packet_t {
  mios32_can_ext_id_t id;
  mios32_can_ctrl_t ctrl;
  mios32_can_data_t data;
} mios32_can_packet_t;

// CAN 16bits filter
typedef struct mios32_can_std_filter_t {
  mios32_can_std_id_t   filt;
  mios32_can_std_id_t   mask;
} mios32_can_std_filter_t;

// CAN 32bits filter
typedef struct mios32_can_ext_filter_t {
  mios32_can_ext_id_t   filt;
  mios32_can_ext_id_t   mask;
} mios32_can_ext_filter_t;

// CAN bus state
typedef enum {
  BUS_OK = 2,
  WARNING = 1,
  PASSIVE = 0,
  BUS_OFF = -1
} mios32_can_bus_stat_t;

// CAN error staus
typedef union {
  struct {
    u32 ALL;
  };
//  struct {
//    u32 rec:8;
//    u32 tec:8;
//    u32 :9;
//    u32 lec:3;
//    u32 :1;
//    u32 boff:1;
//    u32 epvf:1;
//    u32 ewgf:1;
//  };
  struct {
	u32 ewgf:1;
	u32 epvf:1;
	u32 boff:1;
	u32 :1;
	u32 lec:3;
	u32 :9;
	u32 tec:8;
    u32 rec:8;
  };
} mios32_can_stat_err_t;

// CAN status report
typedef struct mios32_can_stat_report_t {
  u32            rx_packets_err;
  u32            tx_packets_ctr;
  u32            rx_packets_ctr;
  u32            rx_buff_err_ctr;
  u8             rx_last_buff_err;
  mios32_can_bus_stat_t bus_state;
  mios32_can_stat_err_t bus_curr_err;
  mios32_can_stat_err_t bus_last_err;
} mios32_can_stat_report_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_CAN_VerboseSet(u8 level);
extern s32 MIOS32_CAN_VerboseGet(void);
extern s32 MIOS32_CAN_Init(u32 mode);

extern s32 MIOS32_CAN_IsAssignedToMIDI(u8 can);

extern s32 MIOS32_CAN_InitPort(u8 can, u8 is_midi);
extern s32 MIOS32_CAN_InitPortDefault(u8 can);
extern s32 MIOS32_CAN_InitPeriph(u8 can);
extern s32 MIOS32_CAN_Init32bitFilter(u8 bank, u8 fifo, mios32_can_ext_filter_t filter, u8 enabled);
extern s32 MIOS32_CAN_Init16bitFilter(u8 bank, u8 fifo, mios32_can_std_filter_t filter1, mios32_can_std_filter_t filter2, u8 enabled);
extern s32 MIOS32_CAN_InitPacket(mios32_can_packet_t *packet);

extern s32 MIOS32_CAN_RxBufferFree(u8 can);
extern s32 MIOS32_CAN_RxBufferUsed(u8 can);
extern s32 MIOS32_CAN_RxBufferGet(u8 can, mios32_can_packet_t *p);
extern s32 MIOS32_CAN_RxBufferPeek(u8 can, mios32_can_packet_t *p);
extern s32 MIOS32_CAN_RxBufferRemove(u8 can);
extern s32 MIOS32_CAN_RxBufferPut(u8 can, mios32_can_packet_t p);

extern s32 MIOS32_CAN_TxBufferFree(u8 can);
extern s32 MIOS32_CAN_TxBufferUsed(u8 can);
extern s32 MIOS32_CAN_TxBufferGet(u8 can, mios32_can_packet_t *p);
extern s32 MIOS32_CAN_TxBufferPutMore_NonBlocking(u8 can, mios32_can_packet_t* p,u16 len);
extern s32 MIOS32_CAN_TxBufferPutMore(u8 can, mios32_can_packet_t *packets, u16 len);
extern s32 MIOS32_CAN_TxBufferPut_NonBlocking(u8 can, mios32_can_packet_t p);
extern s32 MIOS32_CAN_TxBufferPut(u8 can, mios32_can_packet_t p);

extern s32 MIOS32_CAN_BusErrorCheck(u8 can);

extern s32 MIOS32_CAN_Transmit(u8 can, mios32_can_packet_t p, s16 block_time);

extern s32 MIOS32_CAN_ReportLastErr(u8 can, mios32_can_stat_err_t* err);
extern s32 MIOS32_CAN_ReportGetCurr(u8 can, mios32_can_stat_report_t* report);
extern s32 MIOS32_CAN_ReportReset(u8 can);
/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _MIOS32_CAN_H */
