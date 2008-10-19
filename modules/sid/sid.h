// $Id$
/*
 * Header file for MBHP_SID module driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_H
#define _SID_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of SIDs (0-2)
#ifndef SID_NUM
#define SID_NUM 2
#endif


// don't touch this
#define SID_REGS_NUM 32


// pin mapping
#ifndef SID_SCLK_PORT
#define SID_SCLK_PORT         GPIOB
#endif
#ifndef SID_SCLK_PIN
#define SID_SCLK_PIN          GPIO_Pin_6  // J19:SC
#endif

#ifndef SID_RCLK_PORT
#define SID_RCLK_PORT         GPIOC
#endif
#ifndef SID_RCLK_PIN
#define SID_RCLK_PIN          GPIO_Pin_13 // J19:RC1
#endif

#ifndef SID_SER_PORT
#define SID_SER_PORT          GPIOB
#endif
#ifndef SID_SER_PIN
#define SID_SER_PIN           GPIO_Pin_5  // J19:SO
#endif

#ifndef SID_CLK_PORT
#define SID_CLK_PORT          GPIOB
#endif
#ifndef SID_CLK_PIN
#define SID_CLK_PIN           GPIO_Pin_7  // J19:SI  -- PWM configuration has to be adapted if another pin is used
#endif

#ifndef SID_CSN0_PORT
#define SID_CSN0_PORT         GPIOC
#endif
#ifndef SID_CSN0_PIN
#define SID_CSN0_PIN          GPIO_Pin_14 // J19:RC2
#endif

#ifndef SID_CSN1_PORT
#define SID_CSN1_PORT         GPIOC
#endif
#ifndef SID_CSN1_PIN
#define SID_CSN1_PIN          GPIO_Pin_0  // J5A:A0 (tmp.)
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    u8 ALL[7];
  };
  struct {
    u8 frq_l; // don't use u16 to avoid little/big endian issues
    u8 frq_h;
    u8 pw_l;
    u8 pw_h;

    unsigned gate:1;
    unsigned sync:1;
    unsigned ringmod:1;
    unsigned test:1;
    unsigned waveform:4;

    unsigned decay:4;
    unsigned attack:4;
    unsigned release:4;
    unsigned sustain:4;
  };
} sid_voice_t;


typedef union {
  struct {
    u8 ALL[SID_REGS_NUM];
  };
  struct {
#if 0
    // issue: this results into 8 byte packages, but the sid_voice_t union defines 7 bytes
    //  __attribute__((packed)) doesn't help... :-/
    sid_voice_t v[3];
#else
    // workaround: (see also alternative definition below)
    u8 v[3][7];
#endif

    u8 filter_l;
    u8 filter_h;

    unsigned filter_select:4;
    unsigned resonance: 4;
    unsigned volume: 4;
    unsigned filter_mode: 4;

    u8 swinsid_phase[3];
    u8 swinsid_mode[3];
  };
  struct {
    sid_voice_t v1;
  };
  struct {
    u8 dummy_v1[7];
    sid_voice_t v2;
  };
  struct {
    u8 dummy_v1[7];
    u8 dummy_v2[7];
    sid_voice_t v3;
  };
} sid_regs_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_Init(u32 mode);
extern s32 SID_Update(u32 mode);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern sid_regs_t sid_regs[SID_NUM];


#endif /* _SID_H */
