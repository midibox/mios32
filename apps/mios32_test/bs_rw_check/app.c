/*
 * Bankstick r/w check
 * RUNING THIS APPLICATION WILL DESTROY ALL DATA ON YOUR BANKSTICKS!!!
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch / thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_DISPLAY  ( tskIDLE_PRIORITY + 1 )

#ifndef BS_CHECK_NUM_BS
#define BS_CHECK_NUM_BS 5
#endif

// each block is 64 bytes
#ifndef BS_CHECK_NUM_BLOCKS_PER_BS
#define BS_CHECK_NUM_BLOCKS_PER_BS 1024
#endif

// display messages
#define DISPLAY_TASK_DELAYMS 100
#define DISPLAY_TASK_MSG_COUNTDOWN_STARTVALUE 2

// error codes
#define BS_CHECK_ERROR_INIT 1
#define BS_CHECK_ERROR_AVAILABLE 2
#define BS_CHECK_ERROR_SIZE 3
#define BS_CHECK_ERROR_WRITE 4
#define BS_CHECK_ERROR_CHECK_WRITE_FINISHED 5
#define BS_CHECK_ERROR_READ 6
#define BS_CHECK_ERROR_COMPARE 7

// phases
#define BS_CHECK_PHASE_STARTWAIT 1
#define BS_CHECK_PHASE_START 2
#define BS_CHECK_PHASE_INIT 3
#define BS_CHECK_PHASE_WRITE 4
#define BS_CHECK_PHASE_READ 5
#define BS_CHECK_PHASE_FINISHED 16

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

static volatile s8 phase;

static volatile u8 bs;
static volatile u16 block;
static volatile u8 time_read;
static volatile u8 time_write;

static volatile u8 last_error;
static volatile s32 last_error_code;

static volatile u8 buffer[64];

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_Display(void *pvParameters);

static void init_buffer(void);
static void clear_buffer(void);
static s32 check_buffer(void);

/////////////////////////////////////////////////////////////////////////////
// Local functions
/////////////////////////////////////////////////////////////////////////////

static void init_buffer(void){
  u8 i;
  for (i=0;i<64;i++){
    buffer[i] = ((bs*4 + block*2 + i) % 256);
    //if (block==0) MIOS32_MIDI_SendDebugMessage("buffer[%d]=%d",i,buffer[i]);
    }
  }

static void clear_buffer(void){
  u8 i;
  for (i=0;i<64;i++)
    buffer[i] = 0;
  }

static s32 check_buffer(void){
  u8 i,err=0;
  for (i=0;i<64;i++){
    u8 value = (bs*4 + block*2 + i) % 256;
    if(buffer[i] != value){
      MIOS32_MIDI_SendDebugMessage("buffer[%d]=%d, should be %d",i,buffer[i],value);
      err = 1; 
      }
    }
  return err ? 0 : 1;
  }


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void){
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_BOARD_LED_Set(0xffffffff,0);
  phase = BS_CHECK_PHASE_STARTWAIT;
  MIOS32_MIDI_SendDebugMessage("Bankstick r/w check");
  // setup display task
  xTaskCreate(TASK_Display, (signed portCHAR *)"Display", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DISPLAY, NULL);
  MIOS32_DELAY_Init(0);
  }


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
  u8 i;
  u32 size;
  while(1){
    switch(phase){
      case BS_CHECK_PHASE_INIT:
        MIOS32_MIDI_SendDebugMessage("Bankstick check init...");
	bs = 0;
	block = 0;
	last_error = 0;
	last_error_code = 0;
	time_read = 0xff;
	time_write = 0xff;
        // initialize IIC..
	if( (last_error_code = MIOS32_IIC_BS_Init(0)) != 0){
	  last_error =  BS_CHECK_ERROR_INIT;
	  phase = BS_CHECK_PHASE_FINISHED;
          }
        else{
	  for(i = 0;i < BS_CHECK_NUM_BS; i++){
	    if ( (size = MIOS32_IIC_BS_CheckAvailable(i)) == 0) {
	      last_error = BS_CHECK_ERROR_AVAILABLE;
	      last_error_code = i;
              //MIOS32_MIDI_SendDebugMessage("Bankstick %d not available",i);
	      phase = BS_CHECK_PHASE_FINISHED;
              break;
	      }
	    if(size < BS_CHECK_NUM_BLOCKS_PER_BS*64){
	      last_error = BS_CHECK_ERROR_SIZE;
	      last_error_code = i;
              //MIOS32_MIDI_SendDebugMessage("Bankstick %d is too small (%d instead of %d)",
              //  i,size,BS_CHECK_NUM_BLOCKS_PER_BS*64);
	      phase = BS_CHECK_PHASE_FINISHED;
              break;
              }
            } 
          if(!last_error)
            phase = BS_CHECK_PHASE_WRITE;
          }
        break;
      case BS_CHECK_PHASE_WRITE:
        // write blocks
        init_buffer();
        if( (last_error_code = MIOS32_IIC_BS_Write(bs,block*64,(char*)buffer,64)) != 0){
	  last_error =  BS_CHECK_ERROR_WRITE;
	  phase = BS_CHECK_PHASE_FINISHED;
          }
        else{
          //MIOS32_DELAY_Wait_uS(5000);
          //MIOS32_MIDI_SendDebugMessage("Poll CheckWriteFinished begin");
	  do{
	    last_error_code = MIOS32_IIC_BS_CheckWriteFinished(bs);
            //MIOS32_MIDI_SendDebugMessage("p:%d",last_error_code);
            //MIOS32_BOARD_LED_Set(0xffffffff,~ MIOS32_BOARD_LED_Get());
	    } while( last_error_code > 0);
          //MIOS32_MIDI_SendDebugMessage("CheckWriteFinished end");
           //MIOS32_BOARD_LED_Set(0xffffffff,0);
          //MIOS32_DELAY_Wait_uS(5000);
	  if(last_error_code < 0){
	    MIOS32_MIDI_SendDebugMessage("IIC error %d",MIOS32_IIC_LastErrorGet(MIOS32_IIC_BS_PORT));
            last_error =  BS_CHECK_ERROR_CHECK_WRITE_FINISHED;
	    phase = BS_CHECK_PHASE_FINISHED;
	    }
	  else if( ++block >= BS_CHECK_NUM_BLOCKS_PER_BS ){
	    block = 0;
	    if( ++bs >= BS_CHECK_NUM_BS ){
	      bs = 0;
	      phase = BS_CHECK_PHASE_READ;
	      }
	    }
          }
        break;
      case BS_CHECK_PHASE_READ:
        // read blocks
        clear_buffer();
        if( (last_error_code = MIOS32_IIC_BS_Read(bs,block*64,(char*)buffer,64)) != 0){
	  last_error =  BS_CHECK_ERROR_READ;
	  phase = BS_CHECK_PHASE_FINISHED;
          }
	else{
	  if( !check_buffer() ){
	    last_error =  BS_CHECK_ERROR_COMPARE;
	    last_error_code = 0;// no further error specification
	    phase = BS_CHECK_PHASE_FINISHED;	   
	    }
	  else if( ++block >= BS_CHECK_NUM_BLOCKS_PER_BS ){
	    block = 0;
	    if( ++bs >= BS_CHECK_NUM_BS ){
	      bs = 0;
	      phase = BS_CHECK_PHASE_FINISHED;
	      }
	    }
          }
        break;
      }
    }
  }

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background to drive display-output
/////////////////////////////////////////////////////////////////////////////


static void TASK_Display(void *pvParameters){
  char * err_on;
  s32 msg_countdown = 0;
  while(1){
    switch(phase){
      case BS_CHECK_PHASE_STARTWAIT:
        if(msg_countdown != -phase){
          MIOS32_LCD_Clear();
          MIOS32_LCD_CursorSet(0,0);
          MIOS32_LCD_PrintString("Bankstick check");
          MIOS32_LCD_CursorSet(0,1);
          MIOS32_LCD_PrintString("Push a button/key..");
          MIOS32_MIDI_SendDebugMessage("Push any button or MIDI-key to start the check..");
          msg_countdown = -phase;
          }
        break;
      case BS_CHECK_PHASE_START:
        msg_countdown = -phase;
        phase = BS_CHECK_PHASE_INIT;
        break;
      case BS_CHECK_PHASE_WRITE:
        MIOS32_LCD_Clear();
	MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintFormattedString("Write blocks 0x%04X",block);
        if(msg_countdown <= 0){
          MIOS32_MIDI_SendDebugMessage("Write blocks 0x%04X",block);
          msg_countdown = 0;// restart message countdown if it was disabled
          }
        break;
      case BS_CHECK_PHASE_READ:
        MIOS32_LCD_Clear();
	MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintFormattedString("Read blocks 0x%04X",block);
        if(msg_countdown <= 0){
          MIOS32_MIDI_SendDebugMessage("Read blocks 0x%04X",block);
          msg_countdown = 0;// restart message countdown if it was disabled
          }
        break;
      case BS_CHECK_PHASE_FINISHED:
        if(msg_countdown != -phase){// was this stuff already printed/ sent do debug condsole?
          msg_countdown = -phase;// disable further message output in this phase
	  MIOS32_LCD_Clear();
	  MIOS32_LCD_CursorSet(0,0);
	  if(last_error){
	    switch(last_error){
	      case BS_CHECK_ERROR_AVAILABLE:
		err_on = "available";
		break;
	      case BS_CHECK_ERROR_SIZE:
		err_on = "checksize";
		break;
	      case BS_CHECK_ERROR_INIT:
		err_on = "init";
		break;
	      case BS_CHECK_ERROR_WRITE:
		err_on = "write";
		break;
	      case BS_CHECK_ERROR_CHECK_WRITE_FINISHED:
		err_on = "write wait";
		break;
	      case BS_CHECK_ERROR_READ:
		err_on = "read";
		break;
	      case BS_CHECK_ERROR_COMPARE:
		err_on = "compare";
		break;
	      }
	    MIOS32_LCD_PrintFormattedString("Err: %s %d",err_on,last_error_code);
	    MIOS32_LCD_CursorSet(0,1);
	    MIOS32_LCD_PrintFormattedString("BS %d  Block %d",bs,block);
	    MIOS32_MIDI_SendDebugMessage("Error on %s, Error code %d, Bankstick %d, Block %d",
              err_on,last_error_code,bs,block);            
	    }
	  else{
	    MIOS32_LCD_PrintFormattedString("Bankstick check");
	    MIOS32_LCD_CursorSet(0,1);	 
	    MIOS32_LCD_PrintFormattedString("success");
	    MIOS32_MIDI_SendDebugMessage("Banstick check finished successfully!");            
	    }
	  MIOS32_MIDI_SendDebugMessage("Push any button or MIDI-key to re-start the check..");
          }
        break;
      }
    // if msg_countdown is < 0, don't change it (used to stop further message output by phase)
    if(msg_countdown >= 0)
      msg_countdown = msg_countdown ? msg_countdown - 1 : DISPLAY_TASK_MSG_COUNTDOWN_STARTVALUE;
    // wait DISPLAY_TASK_DELAY mS
    vTaskDelay(DISPLAY_TASK_DELAYMS / portTICK_RATE_MS);
    }
  }

/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package){
  if( midi_package.type == NoteOn && midi_package.velocity > 0 )
    phase = BS_CHECK_PHASE_START;  
  }


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value){
  if(!pin_value)
    phase = BS_CHECK_PHASE_START;
  }


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
