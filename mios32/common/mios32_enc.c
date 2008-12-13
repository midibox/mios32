// $Id$
/*
 * Rotary Encoder functions for MIOS32
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_ENC)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:32;
  };
  struct {
    unsigned act1:1;        // current status of pin 1
    unsigned act2:1;        // current status of pin 2
    unsigned last1:1;       // last status of pin 1
    unsigned last2:1;       // last status of pin 2
    unsigned decinc:1;      // 1 if last action was decrement, 0 if increment
    signed   incrementer:8; // the incrementer
    unsigned accelerator:8; // the accelerator for encoder speed detetion
    unsigned predivider:4;  // predivider for slow mode
  };
  struct {
    unsigned act12:2;       // combines act1/act2
    unsigned last12:2;      // combines last1/last2
  };
  struct {
    unsigned state:4;      // combines act1/act2/last1/last2
  };
} enc_state_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

mios32_enc_config_t enc_config[MIOS32_ENC_NUM_MAX];

enc_state_t enc_state[MIOS32_ENC_NUM_MAX];


/////////////////////////////////////////////////////////////////////////////
// Initializes Encoder driver
// IN: <mode>: currently only mode 0 supported
//             later we could provide operation modes
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC_Init(u32 mode)
{
  u8 i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear encoder variables
  for(i=0; i<MIOS32_ENC_NUM_MAX; ++i) {
    enc_config[i].cfg.type = DISABLED; // disable encoder

    enc_state[i].state = 0xf; // all pins released
    enc_state[i].decinc = 0;
    enc_state[i].incrementer = 0;
    enc_state[i].accelerator = 0;
    enc_state[i].predivider = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Configures Encoder
// IN: 
//   <encoder>: encoder number (0..MIOS32_ENC_NUM_MAX-1)
//   <enc_config.cfg.type>: encoder type (DISABLED/NON_DETENTED/DETENTED1..3)
//   <enc_config.cfg.speed: encoder speed mode (NORMAL/FAST/SLOW)
//   <enc_config.cfg.speed_par: speed parameter (0-7)
//   <enc_config.cfg.sr:    shift register (1-16)
//   <enc_config.cfg.pos:   pin position of first pin (0, 2, 4 or 6)
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC_ConfigSet(u32 encoder, mios32_enc_config_t config)
{
  // no SRIOs?
#if MIOS32_SRIO_NUM_SR == 0
  return -1; // no SRIO
#endif

  // encoder number valid?
  if( encoder >= MIOS32_ENC_NUM_MAX )
    return -2; // invalid number

  // take over new configuration
  enc_config[encoder] = config;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns encoder configuration
// IN: <encoder>: encoder number (0..MIOS32_ENC_NUM_MAX-1)
// OUT:
//   <enc_config.cfg.type>: encoder type (DISABLED/NON_DETENTED/DETENTED1..3)
//   <enc_config.cfg.speed: encoder speed mode (NORMAL/FAST/SLOW)
//   <enc_config.cfg.speed_par: speed parameter (0-7)
//   <enc_config.cfg.sr:    shift register (1-16)
//   <enc_config.cfg.pos:   pin position of first pin (0, 2, 4 or 6)
/////////////////////////////////////////////////////////////////////////////
mios32_enc_config_t MIOS32_ENC_ConfigGet(u32 encoder)
{
  // encoder number valid?
  if( encoder >= MIOS32_ENC_NUM_MAX ) {
    const mios32_enc_config_t dummy = { .cfg.type=DISABLED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=0, .cfg.pos=0 };
    return dummy;
  }

  return enc_config[encoder];
}


/////////////////////////////////////////////////////////////////////////////
// This function has to be called after a SRIO scan to update encoder states
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC_UpdateStates(void)
{
  u8 enc;

  // no SRIOs?
#if MIOS32_SRIO_NUM_SR == 0
  return -1; // no SRIO
#endif

  // check all encoders
  // Note: scanning of 64 encoders takes ca. 30 uS @ 72 MHz :-)
  for(enc=0; enc<MIOS32_ENC_NUM_MAX; ++enc) {
    mios32_enc_config_t *enc_config_ptr = &enc_config[enc];

    // skip if encoder not configured
    if( enc_config_ptr->cfg.type == DISABLED )
      continue;

    enc_state_t *enc_state_ptr = &enc_state[enc];

    // decrement accelerator until it is zero (used to determine rotation speed)
    if( enc_state_ptr->accelerator )
      --enc_state_ptr->accelerator;

    // check if encoder state has been changed, and clear changed flags, so that the changes won't be propagated to DIN handler
    u8 sr = enc_config_ptr->cfg.sr-1;
    u8 pos = enc_config_ptr->cfg.pos;
    u8 changed_mask = 3 << pos; // note: by checking mios32_srio_din_changed[sr] directly, we speed up the scanning of unmoved encoders by factor 3!
    if( (mios32_srio_din_changed[sr] & changed_mask) && MIOS32_DIN_SRChangedGetAndClear(sr, changed_mask) ) {
      mios32_enc_type_t enc_type = enc_config_ptr->cfg.type;
      enc_state_ptr->last12 = enc_state_ptr->act12;
      enc_state_ptr->act12 = (MIOS32_DIN_SRGet(sr) >> pos) & 3;
      s32 predivider;
      s32 acc;

      // State Machine (own Design from 1999)
      // changed 2000-1-5: special "analyse" state which corrects the ENC direction
      // if encoder is rotated to fast - I should patent it ;-)

      // NON_DETENTED INC: 2, B, D, 4
      //              DEC: 1, 7, E, 8
      // DETENTED1    INC: B, 4
      //              DEC: 7, 8
      // DETENTED2    INC: B
      //              DEC: 7
      // DETENTED3    INC: 4
      //              DEC: 8
      switch( enc_state_ptr->state ) {
        case 0x0: goto MIOS32_ENC_Do_Nothing;	// 0
	case 0x1: goto MIOS32_ENC_Do_Dec_ND;	// 1  -  only if NON_DETENTED
	case 0x2: goto MIOS32_ENC_Do_Inc_ND;	// 2  -  only if NON_DETENTED
	case 0x3: goto MIOS32_ENC_Do_Nothing;	// 3
	case 0x4: goto MIOS32_ENC_Do_Inc_D13;	// 4  -  only if NON_DETENTED, DETENTED1 or DETENTED3
	case 0x5: goto MIOS32_ENC_Do_Nothing;	// 5
	case 0x6: goto MIOS32_ENC_Do_Nothing;	// 6
	case 0x7: goto MIOS32_ENC_Do_Dec_D12;	// 7  -  only if NON_DETENTED, DETENTED1 or DETENTED2
	case 0x8: goto MIOS32_ENC_Do_Dec_D13;	// 8  -  only if NON_DETENTED, DETENTED1 or DETENTED3
	case 0x9: goto MIOS32_ENC_Do_Nothing;	// 9
	case 0xa: goto MIOS32_ENC_Do_Nothing;	// A
	case 0xb: goto MIOS32_ENC_Do_Inc_D12;	// B  -  only if NON_DETENTED, DETENTED1 or DETENTED2
	case 0xc: goto MIOS32_ENC_Do_Nothing;	// C
	case 0xd: goto MIOS32_ENC_Do_Inc_ND;	// D  -  only if NON_DETENTED
	case 0xe: goto MIOS32_ENC_Do_Dec_ND;	// E  -  only if NON_DETENTED
	case 0xf: goto MIOS32_ENC_Do_Nothing;	// F
        default:  goto MIOS32_ENC_Do_Nothing;
      }

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Dec_ND:
      // only if NON_DETENTED
      if( enc_type == NON_DETENTED )
	goto MIOS32_ENC_Do_Dec;
      goto MIOS32_ENC_Do_Nothing;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Inc_ND:
      // only if NON_DETENTED
      if( enc_type == NON_DETENTED )
	goto MIOS32_ENC_Do_Inc;
      goto MIOS32_ENC_Do_Nothing;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Dec_D12:
      // only if NON_DETENTED, DETENTED1 or DETENTED2
      if( enc_type != DETENTED3 )
	goto MIOS32_ENC_Do_Dec;
      goto MIOS32_ENC_Do_Nothing;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Inc_D12:
      // only if NON_DETENTED, DETENTED1 or DETENTED2
      if( enc_type != DETENTED3 )
	goto MIOS32_ENC_Do_Inc;
      goto MIOS32_ENC_Do_Nothing;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Dec_D13:
      // only if NON_DETENTED, DETENTED1 or DETENTED3
      if( enc_type != DETENTED2 )
	goto MIOS32_ENC_Do_Dec;
      goto MIOS32_ENC_Do_Nothing;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Inc_D13:
      // only if NON_DETENTED, DETENTED1 or DETENTED3
      if( enc_type != DETENTED2 )
	goto MIOS32_ENC_Do_Inc;
      goto MIOS32_ENC_Do_Nothing;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Inc:
      // plausibility check: when accelerator > 0xe0, exit if last event was a DEC
      if( enc_state_ptr->decinc && enc_state_ptr->accelerator > 0xe0 )
	goto MIOS32_ENC_Do_Nothing;

      // memorize INC
      enc_state_ptr->decinc = 0;

      // branch depending on speed mode
      switch( enc_config_ptr->cfg.speed ) {
        case FAST:
	  if( (acc=(enc_state_ptr->accelerator >> (7-enc_config_ptr->cfg.speed_par))) == 0 )
	    acc = 1;
          enc_state_ptr->incrementer += acc;
	  break;

        case SLOW:
	  predivider = enc_state_ptr->predivider + (enc_config_ptr->cfg.speed_par+1);
	  // increment on 4bit overrun
	  if( predivider >= 16 )
	    ++enc_state_ptr->incrementer;
	  enc_state_ptr->predivider = predivider;
	  break;

        default: // NORMAL
          ++enc_state_ptr->incrementer;
	  break;
      }

      goto MIOS32_ENC_Next;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Dec:
      // plausibility check: when accelerator > 0xe0, exit if last event was a INC
      if( !enc_state_ptr->decinc && enc_state_ptr->accelerator > 0xe0 )
	goto MIOS32_ENC_Do_Nothing;

      // memorize DEC
      enc_state_ptr->decinc = 1;

      // branch depending on speed mode
      switch( enc_config_ptr->cfg.speed ) {
        case FAST:
	  if( (acc=(enc_state_ptr->accelerator >> (7-enc_config_ptr->cfg.speed_par))) == 0 )
	    acc = 1;
          enc_state_ptr->incrementer -= acc;
	  break;

        case SLOW:
	  predivider = enc_state_ptr->predivider - (enc_config_ptr->cfg.speed_par+1);
	  // increment on 4bit underrun
	  if( predivider < 0 )
	    --enc_state_ptr->incrementer;
	  enc_state_ptr->predivider = predivider;
	  break;

        default: // NORMAL
          --enc_state_ptr->incrementer;
	  break;
      }

      goto MIOS32_ENC_Next;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Next:
      // set accelerator to max value (will be decremented on each tick, so that the encoder speed can be determined)
      enc_state_ptr->accelerator = 0xff;

      //////////////////////////////////////////////////////////////////////////
MIOS32_ENC_Do_Nothing:
      if( 0 ); // dummy to prevent "error: label at end of compound statement"
    }

  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This handler checks for encoder movements, and calls the given callback
// function with following parameters:
//   - u32 encoder
//   - s32 incrementer
//
// Example: MIOS32_ENC_Handler(ENC_NotifyToggle);
//          will call
//          void ENC_NotifyToggle(u32 encoder, s32 incrementer)
//          on encoer movements
// IN: pointer to callback function
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC_Handler(void *_callback)
{
  u8 enc;
  s32 incrementer;
  void (*callback)(u32 pin, u32 value) = _callback;

  // no SRIOs?
#if MIOS32_SRIO_NUM_SR == 0
  return -1; // no SRIO
#endif

  // no callback function?
  if( _callback == NULL )
    return -1;

  // check all encoders
  for(enc=0; enc<MIOS32_ENC_NUM_MAX; ++enc) {

    // following check/modify operation must be atomic
    MIOS32_IRQ_Disable();
    if( incrementer = enc_state[enc].incrementer ) {
      enc_state[enc].incrementer = 0;
      MIOS32_IRQ_Enable();

      // call the hook
      callback(enc, incrementer);
    } else {
      MIOS32_IRQ_Enable();
    }
  }

  return 0; // no error
}


#endif /* MIOS32_DONT_USE_ENC */
