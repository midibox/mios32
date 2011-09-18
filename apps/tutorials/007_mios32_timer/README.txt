$Id$

MIOS32 Tutorial #007: MIOS32 Timer
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17

===============================================================================

In the previous lesson we learnt the usage of FreeRTOS tasks to process a
routine periodically. Now we will get in touch with an alternative method
which allows more precise time control: MIOS32_Timer

MIOS32 provides an application layer to three hardware controlled timers
with an accuracy of one Millisecond (!)
Supported interval range is 1 uS .. 65535 uS (ca. 65.6 mS)

Timer routines are usually running with a higher priority than any FreeRTOS
task. They should should be executed so fast as possible (use only short
routines), so that the performance of the main application isn't affected.

The usage of timers can get dangerous (lead to application hang-ups) if
the execution time is too long, and/or if they are triggered to often
so that RTOS tasks are "starving".

If this should every happen during your experiments, you will probably
get no response from the USB interface anymore, accordingly it won't be
possible to run a "Query" or to upload new code.
Mostly the only solution to get out of this situation is to select
MIOS32 Bootloader Hold Mode (stuff Jumper J27) and to power-cycle
the core as explained under:
  http://www.ucapps.de/mios32_bootstrap_newbies.html


However, please don't fear such a "danger" on the following experiment.
The timer will be executed each 100 uS for ca. 1 uS (accordingly only
1% of CPU time is consumed) - these are safe conditions! :-)


A nice demonstrator for a timer function is a "PWM generator":
We want to dimm the board LED with the modulation wheel of a MIDI keyboard
(or with the virtual keyboard of MIOS Studio)


First we have to initialize the timer (we take the first one #0), so 
that it calls PWM_Timer() each 100 uS.

This is done in APP_Init():
-------------------------------------------------------------------------------
  // initialize MIOS32 Timer #0, so that our PWM_Timer() function is
  // called each 100 uS:
  MIOS32_TIMER_Init(0, 100, PWM_Timer, MIOS32_IRQ_PRIO_MID);
-------------------------------------------------------------------------------

The function parameters are explained under:
  http://www.midibox.org/mios32/manual/group___m_i_o_s32___t_i_m_e_r.html



The timer function itself will increment a counter variable each 100 uS.
Once the counter reached 128, it will be reset.
This leads to a period of 100 uS * 128 = 12.8 mS

And thats the PWM period of the dimmed LED. The LED will be turned on between
0..pwm_duty, which means: than higher pwm_duty, than brighter the LED:
-------------------------------------------------------------------------------
static void PWM_Timer(void)
{
  if( ++pwm_counter >= 128 ) {
    // reset counter once it reached 128
    pwm_counter = 0;
    // turn on status LED
    MIOS32_BOARD_LED_Set(1, 1);
  }

  // turn off status LED once we reached PWM value controlled by Mod Wheel
  if( pwm_counter == pwm_duty )
    MIOS32_BOARD_LED_Set(1, 0);
}
-------------------------------------------------------------------------------



The LED brightness (-> pwm_duty) is controlled
-------------------------------------------------------------------------------


===============================================================================
