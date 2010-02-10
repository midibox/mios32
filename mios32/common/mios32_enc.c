// $Id$
//! \defgroup MIOS32_ENC
//!
//! Rotary Encoder functions for MIOS32
//!
//! \{
/* ==========================================================================
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
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  //commented this out because it produced a compilation error and does not seem to be needed
	/*struct {
	 unsigned ALL:41;
	 };*/
  struct {
    unsigned act1:1;						// current status of pin 1
    unsigned act2:1;						// current status of pin 2
    unsigned last1:1;						// last status of pin 1
    unsigned last2:1;						// last status of pin 2
    unsigned decinc:1;					// 1 if last action was decrement, 0 if increment
    signed   incrementer:8;			// the incrementer
    unsigned accelerator:8;			// the accelerator for encoder speed detetion
    unsigned prev_state_dec:4;	//last INC state
		unsigned prev_state_inc:4;	// last DEC state	
		unsigned prev_acc:8;				//last acceleration value, for smoothing out sudden acceleration changes
		unsigned predivider:4;			//predivider for SLOW mode
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
//! Initializes encoder driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
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
		enc_state[i].prev_state_dec = 0;
		enc_state[i].prev_state_inc = 0;
		enc_state[i].prev_acc = 0;
		enc_state[i].predivider = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Configures Encoder
//! \param[in] encoder encoder number (0..MIOS32_ENC_NUM_MAX-1)
//! \param[in] config a structure with following members:
//! <UL>
//!   <LI>enc_config.cfg.type: encoder type (DISABLED/NON_DETENTED/DETENTED1..3)<BR>
//!   <LI>enc_config.cfg.speed encoder speed mode (NORMAL/FAST/SLOW)<BR>
//!   <LI>enc_config.cfg.speed_par speed parameter (0-7)<BR>
//!   <LI>enc_config.cfg.sr shift register (1-16)<BR>
//!   <LI>enc_config.cfg.pos pin position of first pin (0, 2, 4 or 6)<BR>
//! </UL>
//! \return < 0 if initialisation failed
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
//! Returns encoder configuration
//! \param[in] encoder encoder number (0..MIOS32_ENC_NUM_MAX-1)
//! \return enc_config.cfg.type encoder type (DISABLED/NON_DETENTED/DETENTED1..3)
//! \return enc_config.cfg.speed encoder speed mode (NORMAL/FAST/SLOW)
//! \return enc_config.cfg.speed_par speed parameter (0-7)
//! \return enc_config.cfg.sr shift register (1-16)
//! \return enc_config.cfg.pos pin position of first pin (0, 2, 4 or 6)
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
//! This function has to be called after a SRIO scan to update encoder states
//! \return < 0 on errors
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
      // changed 2009-09-14: new ENC_MODE-format, using Bits of ENC_MODE_xx to
      // indicate edges, which trigger Do_Inc / Do_Dec
			
      // if Bit N of ENC_MODE is set, according ENC_STAT triggers Do_Inc / Do_Dec
      //
      // Bit N     7   6   5   4  
      // ENC_STAT  8   E   7   1
      // DEC      <-  <-  <-  <-  
      // Pin A ____|-------|_______
      // Pin B ________|-------|___
      // INC       ->  ->  ->  ->  
      // ENC_STAT  2   B   D   4
      // Bit N     0   1   2   3 
      // This method is based on ideas from Avogra
			
			if( (enc_state_ptr->state == 0x01 && (enc_type & (1 << 4))) ||
				 (enc_state_ptr->state == 0x07 && (enc_type & (1 << 5))) ||
				 (enc_state_ptr->state == 0x0e && (enc_type & (1 << 6))) ||
				 (enc_state_ptr->state == 0x08 && (enc_type & (1 << 7))) ) {
				// DEC
				// plausibility check: when accelerator > 0xe0, exit if last event was a INC. Also, only do anything if the state has actually changed
				if( (enc_state_ptr->decinc || enc_state_ptr->accelerator <= 0xe0) && enc_state_ptr->state != enc_state_ptr->prev_state_dec ) {
					// memorize DEC
					enc_state_ptr->decinc = 1;
					
					//limit maximum increase of accelerator
					if (enc_state_ptr->accelerator - enc_state_ptr->prev_acc > 20) {
						enc_state_ptr->accelerator = enc_state_ptr->prev_acc + 20;
					}
					
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
					//save last acceleration value
					enc_state_ptr->prev_acc = enc_state_ptr->accelerator;
					
					// set accelerator to max value (will be decremented on each tick, so that the encoder speed can be determined)
					enc_state_ptr->accelerator = 0xff;
					
					//save last state to compare whether the state changed in the next run
					enc_state_ptr->prev_state_dec = enc_state_ptr->state;
				}
			} else if( (enc_state_ptr->state == 0x02 && (enc_type & (1 << 0))) ||
								(enc_state_ptr->state == 0x0b && (enc_type & (1 << 1))) ||
								(enc_state_ptr->state == 0x0d && (enc_type & (1 << 2))) ||
								(enc_state_ptr->state == 0x04 && (enc_type & (1 << 3))) ) {
				// INC
				// plausibility check: when accelerator > 0xe0, exit if last event was a DEC. Also, only do anything if the state has actually changed
				if( (!enc_state_ptr->decinc || enc_state_ptr->accelerator <= 0xe0) && enc_state_ptr->state != enc_state_ptr->prev_state_inc ) {
					// memorize INC
					enc_state_ptr->decinc = 0;
					
					//limit maximum increase of accelerator
					if (enc_state_ptr->accelerator - enc_state_ptr->prev_acc > 20) {
						enc_state_ptr->accelerator = enc_state_ptr->prev_acc + 20;
					}
					
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
					//save last acceleration value
					enc_state_ptr->prev_acc = enc_state_ptr->accelerator;
					
					// set accelerator to max value (will be decremented on each tick, so that the encoder speed can be determined)
					enc_state_ptr->accelerator = 0xff;
					
					//save last state to compare whether the state changed in the next run
					enc_state_ptr->prev_state_inc = enc_state_ptr->state;
				}
			}
		}
	}	
return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This handler checks for encoder movements, and calls the given callback
//! function with following parameters on encoder movements:
//! \code
//!   void ENC_NotifyToggle(u32 encoder, s32 incrementer)
//! \endcode
//! \param[in] _callback pointer to callback function
//! \return < 0 on errors
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
    if( (incrementer = enc_state[enc].incrementer) ) {
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

//! \}

#endif /* MIOS32_DONT_USE_ENC */
