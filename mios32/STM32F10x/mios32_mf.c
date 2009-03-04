// $Id$
//! \defgroup MIOS32_MF
//!
//! Motorfader functions for MIOS32
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
#if !defined(MIOS32_DONT_USE_MF)


/////////////////////////////////////////////////////////////////////////////
// Local defines/macros
/////////////////////////////////////////////////////////////////////////////

#define REPEAT_CTR_RELOAD         31  // retries to reach target position
#define TIMEOUT_CTR_RELOAD       255 // give up after how many mS
#define MANUAL_MOVE_CTR_RELOAD   255 // ignore new position request for how many mS


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL0:32;
    unsigned ALL1:32;
    unsigned ALL2:32;
  };
  struct {
    unsigned up:1;
    unsigned down:1;
    unsigned suspended:1;
    unsigned idle:1;
    unsigned direct_control:1;
    unsigned dummy:3+8; // fill to 16bit

    u16 pos;

    u8  pwm_ctr;
    u8  manual_move_ctr;
    u8  timeout_ctr;
    u8  repeat_ctr;

    mios32_mf_config_t config;
  };
  struct {
    mios32_mf_direction_t direction;
  };
} mf_state_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_MF_NUM

// control variables for motorfaders
static mf_state_t mf_state[MIOS32_MF_NUM];

#endif


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_MF_UpdateSR(void);


/////////////////////////////////////////////////////////////////////////////
//! Initializes motorfader driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if !MIOS32_MF_NUM
  return -1; // no motorfaders
#else

  // initial state of RCLK
  MIOS32_SPI_RC_PinSet(MIOS32_MF_SPI, MIOS32_MF_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // init GPIO structure
  MIOS32_SPI_IO_Init(MIOS32_MF_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);

  // init SPI port for fast baudrate
  MIOS32_SPI_TransferModeInit(MIOS32_MF_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);

  int i;
  for(i=0; i<MIOS32_MF_NUM; ++i) {
    mf_state[i].ALL0 = 0;
    mf_state[i].ALL1 = 0;
    mf_state[i].ALL2 = 0;

    mf_state[i].config.cfg.deadband = 15;
    mf_state[i].config.cfg.pwm_period = 3;
    mf_state[i].config.cfg.pwm_duty_cycle_down = 1;
    mf_state[i].config.cfg.pwm_duty_cycle_up = 1;
  }

  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! set target position and move fader
//! \param[in] mf motor number (0..MIOS32_MF_NUM-1)
//! \param[in] pos motorfader position 
//!      (resolution depends on the used AIN resolution, usually 12bit)
//! \return -1 if motor doesn't exist
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_FaderMove(u32 mf, u16 pos)
{
#if !MIOS32_MF_NUM
  return -1; // no motors
#else
  // check if motor exists
  if( mf >= MIOS32_MF_NUM )
    return -1;

  // skip if fader currently manually moved (feedback killer)
  if( mf_state[mf].manual_move_ctr )
    return 0; // no error

  // skip if current position already inside MF deadband (we cannot do it better anyhow...)
  s32 mf_delta = MIOS32_AIN_PinGet(mf) - pos;
  if( abs(mf_delta) < mf_state[mf].config.cfg.deadband )
    return 0; // no error

  // following sequence must be atomic
  MIOS32_IRQ_Disable();

  // set new motor position
  mf_state[mf].pos = pos;

  // reinit repeat and timeout counter
  mf_state[mf].repeat_ctr = REPEAT_CTR_RELOAD;
  mf_state[mf].timeout_ctr = TIMEOUT_CTR_RELOAD;

  MIOS32_IRQ_Enable();

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! direct control over the motorfader
//! \param[in] mf motor number (0..MIOS32_MF_NUM-1)
//! \param[in] direction is MF_Standby, MF_Up or MF_Down
//! \return -1 if motor doesn't exist
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_FaderDirectMove(u32 mf, mios32_mf_direction_t direction)
{
#if !MIOS32_MF_NUM
  return -1; // no motors
#else
  // check if motor exists
  if( mf >= MIOS32_MF_NUM )
    return -1;

  // set new motor direction (must be atomic)
  MIOS32_IRQ_Disable();
  mf_state[mf].direction = direction;
  mf_state[mf].direct_control = (direction == MF_Standby) ? 0 : 1;
  MIOS32_IRQ_Enable();

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! activates/deactivates suspend mode of motor<BR>
//! (function used by touchsensor detection)
//! \param[in] mf motor number (0..MIOS32_MF_NUM-1)
//! \param[in] suspend 1 to enable, 0 to disable suspend
//! \return -1 if motor doesn't exist
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_SuspendSet(u32 mf, u8 suspend)
{
#if !MIOS32_MF_NUM
  return -1; // no motors
#else
  // check if motor exists
  if( mf >= MIOS32_MF_NUM )
    return -1;

  if( suspend ) {
    mf_state[mf].suspended = 1;
  } else {
    mf_state[mf].suspended = 0;
    mf_state[mf].manual_move_ctr = MANUAL_MOVE_CTR_RELOAD;
  }

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns the suspend state of the motor
//! \param[in] mf motor number (0..MIOS32_MF_NUM-1)
//! \return -1 if motor doesn't exist
//! \return 1 if motor suspended
//! \return 0 if motor not suspended
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_SuspendGet(u32 mf)
{
#if !MIOS32_MF_NUM
  return -1; // no motors
#else
  // check if motor exists
  if( mf >= MIOS32_MF_NUM )
    return -1;

  return mf_state[mf].suspended;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! this function resets the software implemented touch detection, so that the
//! fader is repositioned regardless if it is currently moved or not
//! \param[in] mf motor number (0..MIOS32_MF_NUM-1)
//! \return -1 if motor doesn't exist
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_TouchDetectionReset(u32 mf)
{
#if !MIOS32_MF_NUM
  return -1; // no motors
#else
  // check if motor exists
  if( mf >= MIOS32_MF_NUM )
    return -1;

  mf_state[mf].manual_move_ctr = 0;

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function configures various MF driver parameters.<BR>
//! see http://www.ucapps.de/mbhp_mf.html for detailed informations about
//! these parameters.
//! \param[in] mf motor number (0..MIOS32_MF_NUM-1)
//! \param[in] config a structure with following members:
//! <UL>
//!   <LI>mf_config.deadband
//!   <LI>mf_config.pwm_period
//!   <LI>mf_config.pwm_duty_cycle_up
//!   <LI>mf_config.pwm_duty_cycle_down
//! </UL>
//! \return -1 if motor doesn't exist
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_ConfigSet(u32 mf, mios32_mf_config_t config)
{
#if !MIOS32_MF_NUM
  return -1; // no motors
#else
  // check if motor exists
  if( mf >= MIOS32_MF_NUM )
    return -1;

  // take over new configuration
  mf_state[mf].config = config;

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns the MF configuration
//! \param[in] mf motor number (0..MIOS32_MF_NUM-1)
//! \return mf_config.deadband
//! \return mf_config.pwm_period
//! \return mf_config.pwm_duty_cycle_up
//! \return mf_config.pwm_duty_cycle_down
/////////////////////////////////////////////////////////////////////////////
mios32_mf_config_t MIOS32_MF_ConfigGet(u32 mf)
{
#if !MIOS32_MF_NUM
  // return 0; // doesn't work :-/
  // no motors
#else
  // MF number valid?
  if( mf >= MIOS32_MF_NUM ) {
    const mios32_mf_config_t dummy = { .cfg.deadband=3, .cfg.pwm_period=3, .cfg.pwm_duty_cycle_up=1, .cfg.pwm_duty_cycle_down=1 };
    return dummy;
  }

  return mf_state[mf].config;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Called from AIN DMA interrupt whenever new conversion results are available
//! \param[in] *ain_values pointer to current conversion results
//! \param[in] *ain_deltas pointer to differences between old and new results
//! \return -1 on errors
//! \return >= 0: mask of 16 "changed" flags which should not cleared
//!               (if "changed" flag set, the AIN driver will propagate a new
//!                conversion value to the application hook)
//! \note shouldn't be called directly from application
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MF_Tick(u16 *ain_values, u16 *ain_deltas)
{
#if !MIOS32_MF_NUM
  return -1; // no motors
#else
  int i;
  u32 change_flag_mask = 0x0000; // by default, request to clear all change flags

  // check all motorfaders
  mf_state_t *mf = (mf_state_t *)&mf_state;
  for(i=0; i<MIOS32_MF_NUM; ++i) {
    // skip if fader directly controlled
    if( !mf->direct_control ) {
      u16 current_pos = ain_values[i];
      u16 ain_delta = ain_deltas[i];
      u8  clear_change_flag = 1; // by default clear AIN change flag!
      
      // counter handling
      if( mf->manual_move_ctr )
	--mf->manual_move_ctr;
      if( mf->timeout_ctr )
	--mf->timeout_ctr;
      
      // check touch detection, shutdown motor if active
      if( mf->suspended ) {
        // no repeats
        mf->repeat_ctr = 0;
        // no timeout
        mf->timeout_ctr = 0;
      }
  
      // check motor state
      mf->idle = 0;
  
      // CASE: motor on target position?
      if( !mf->repeat_ctr ) {
        // motor should go into standby mode
        mf->direction = MF_Standby;
        mf->idle = 1;
  
        // timeout reached?
        if( mf->timeout_ctr ) {
	  // no: copy current AIN value into target position (reassurance phase)
	  mf->pos = current_pos;
        } else {
	  // AIN value was outside deadband?
	  if( ain_delta > MIOS32_AIN_DEADBAND ) {
	    // copy current AIN value into target position
	    mf->pos = current_pos;
	    // set manual move counter, so that the motor won't be moved during this time
	    mf->manual_move_ctr = MANUAL_MOVE_CTR_RELOAD;
	    // change flag should not be cleared
	    change_flag_mask |= (1 << i);
	  }
        }
      }
      // CASE: motor very slow or not moving
      else if( !mf->timeout_ctr && ain_delta <= MIOS32_AIN_DEADBAND ) {
        // if timeout reached, write 1 into repeat counter for proper shutdown
        if( mf->timeout_ctr == 0 ) {
	  mf->repeat_ctr = 1;
	  mf->idle = 1;
        }
      }
      // CASE: motor is moving fast
      else if( ain_delta > MIOS32_AIN_DEADBAND ) {
        // fine: reload timeout counter
        mf->timeout_ctr = TIMEOUT_CTR_RELOAD;
      }
      
      // continue if motor control hasn't reached idle state
      if( !mf->idle ) {
        // don't move motor if speed too fast
        if( ain_delta > 500 ) { // TODO: check with panasonic faders
	  mf->direction = MF_Standby;
        } else {
	  // determine into which direction the motor should be moved
	  
	  // special case: if target and current position <= 0x01f, stop motor
	  // (workaround for ALPS faders which never reach the 0x000 value)
	  if( mf->pos < 0x1f && current_pos < 0x1f ) {
	    mf->direction = MF_Standby;
	    --mf->repeat_ctr;
	  } else {
	    // check distance between current and target position
	    // if fader is in between the MF deadband, don't move it anymore
	    s32 mf_delta = current_pos - mf->pos;
	    u32 abs_mf_delta = abs(mf_delta);
	    
	    // dynamic deadband: depends on repeat counter
	    u8 dyn_deadband;
	    if( mf->repeat_ctr < 4 )
	      dyn_deadband = 16;
	    else if( mf->repeat_ctr < 8 )
	      dyn_deadband = 32;
	    else if( mf->repeat_ctr < 16 )
	      dyn_deadband = 64;
	    else
	      dyn_deadband = mf->config.cfg.deadband;
	    
	    if( abs_mf_delta <= dyn_deadband ) {
	      mf->direction = MF_Standby;
	      --mf->repeat_ctr;
	    } else {
	      // slow down motor via PWM if distance between current and target position < 0x180
	      if( mf->config.cfg.pwm_period && abs_mf_delta < 0x180 ) {
		if( ++mf->pwm_ctr > mf->config.cfg.pwm_period )
		  mf->pwm_ctr = 0;
		
		if( mf->pwm_ctr > ((mf_delta > 0) ? mf->config.cfg.pwm_duty_cycle_up : mf->config.cfg.pwm_duty_cycle_up) ) {
		  mf->idle = 1;
		}
	      }
	    }
	    
	    // check if motor should be moved up/down
	    if( mf->idle )
	      mf->direction = MF_Standby;
	    else
	      mf->direction = (mf_delta > 0) ? MF_Up : MF_Down;
	  }
        }
      }
    }
    
    // switch to next motorfader
    ++mf;
  }

  // update H bridges
  MIOS32_MF_UpdateSR();

  // notify AIN driver which change flags should be cleared
  return change_flag_mask; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
// local function to update the shift registers of MBHP_MF module
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_MF_NUM
static void MIOS32_MF_UpdateSR(void)
{
  int i;

  // shift in 8bit data
#if MIOS32_MF_NUM > 8
  for(i=12; i>=0; i-=4) { // 2 MBHP_MF modules
#else
  for(i=4; i>=0; i-=4) { // 1 MBHP_MF module
#endif
    u8 buffer = 0;

    mf_state_t *mf_state_ptr = (mf_state_t *)&mf_state[i];
    if( mf_state_ptr->up )
      buffer |= 0x01;
    if( mf_state_ptr->down )
      buffer |= 0x02;

    mf_state_ptr++;
    if( mf_state_ptr->up )
      buffer |= 0x04;
    if( mf_state_ptr->down )
      buffer |= 0x08;

    mf_state_ptr++;
    if( mf_state_ptr->up )
      buffer |= 0x10;
    if( mf_state_ptr->down )
      buffer |= 0x20;

    mf_state_ptr++;
    if( mf_state_ptr->up )
      buffer |= 0x40;
    if( mf_state_ptr->down )
      buffer |= 0x80;

    MIOS32_SPI_TransferByte(MIOS32_MF_SPI, buffer);
  }

  // transfer to output register
  MIOS32_SPI_RC_PinSet(MIOS32_MF_SPI, MIOS32_MF_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
  MIOS32_SPI_RC_PinSet(MIOS32_MF_SPI, MIOS32_MF_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
}
#endif

//! \}

#endif /* MIOS32_DONT_USE_MF */
