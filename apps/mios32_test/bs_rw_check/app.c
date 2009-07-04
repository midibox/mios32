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
#include <fram.h>

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_DISPLAY  ( tskIDLE_PRIORITY + 1 )

#ifndef BS_CHECK_NUM_DEVICES
#define BS_CHECK_NUM_DEVICES 5
#endif

// data block size (max. 64 if BS_CHECK_USE_FRAM_LAYER==0)
#ifndef BS_CHECK_DATA_BLOCKSIZE
#define BS_CHECK_DATA_BLOCKSIZE 64
#endif


// data blocks per device
#ifndef BS_CHECK_NUM_BLOCKS_PER_DEVICE
#define BS_CHECK_NUM_BLOCKS_PER_DEVICE 1024
#endif

// fram layer usage
#ifndef BS_CHECK_USE_FRAM_LAYER
#define BS_CHECK_USE_FRAM_LAYER 0
#endif

// display messages
#define DISPLAY_TASK_DELAYMS 100
#define DISPLAY_TASK_MSG_COUNTDOWN_STARTVALUE 2

// error codes
#define BS_CHECK_ERROR_INIT 1
#define BS_CHECK_ERROR_AVAILABLE 2
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


// limit blocksize for banksticks to 64 bytes
#if (BS_CHECK_USE_FRAM_LAYER == 0 && BS_CHECK_DATA_BLOCKSIZE > 64)
#define BS_CHECK_DATA_BLOCKSIZE 64
#endif


// calculation of a buffer byte in respect to current bankstick, buffer and byte(i)
#define BS_CHECK_BUFFER_BYTE_VALUE ((run + bs + block+(block/3) + i) % 256)

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

static volatile s8 phase;

static volatile u8 bs;
static volatile u16 block;
static volatile u32 run;
static volatile u8 time_read;
static volatile u8 time_write;

static volatile u8 last_error;
static volatile s32 last_error_code;

static volatile u8 buffer[BS_CHECK_DATA_BLOCKSIZE];


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_Display(void *pvParameters);

static void init_buffer(void);
static void clear_buffer(void);
static s32 check_buffer(void);

static s32 wait_write_finished(void);
static void report_iic_error(void);

/////////////////////////////////////////////////////////////////////////////
// Local functions
/////////////////////////////////////////////////////////////////////////////

static void init_buffer(void){
  u8 i;
  for (i=0;i<BS_CHECK_DATA_BLOCKSIZE;i++){
    buffer[i] = BS_CHECK_BUFFER_BYTE_VALUE;
    //if (block==0) MIOS32_MIDI_SendDebugMessage("buffer[%d]=%d",i,buffer[i]);
    }
  }

static void clear_buffer(void){
  u8 i;
  for (i=0;i<BS_CHECK_DATA_BLOCKSIZE;i++)
    buffer[i] = 0;
  }

static s32 check_buffer(void){
  u8 i,err=0;
  for (i=0;i<BS_CHECK_DATA_BLOCKSIZE;i++){
    u8 value = BS_CHECK_BUFFER_BYTE_VALUE;
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
	bs = 0;
	block = 0;
	last_error = 0;
	last_error_code = 0;
	time_read = 0xff;
	time_write = 0xff;
        // initialize IIC..
        if(run == 0){
	  MIOS32_MIDI_SendDebugMessage("Bankstick check init...");
#if BS_CHECK_USE_FRAM_LAYER == 0
	  if( (last_error_code = MIOS32_IIC_BS_Init(0)) != 0){
#else
	  if( (last_error_code = FRAM_Init(0)) != 0){ 
#endif
	    last_error =  BS_CHECK_ERROR_INIT;
	    phase = BS_CHECK_PHASE_FINISHED;
	    }
	  else{
	    for(i = 0;i < BS_CHECK_NUM_DEVICES; i++){
#if BS_CHECK_USE_FRAM_LAYER == 0
	      if ( (size = MIOS32_IIC_BS_CheckAvailable(i)) == 0) {
#else
	      if ( (size = FRAM_CheckAvailable(i)) == 0) {
#endif
		report_iic_error();
		last_error = BS_CHECK_ERROR_AVAILABLE;
		last_error_code = i;
		MIOS32_MIDI_SendDebugMessage("Bankstick %d not available",i);
		phase = BS_CHECK_PHASE_FINISHED;
		break;
		}
	      } 
            }
          }
        else{
          MIOS32_MIDI_SendDebugMessage("Start next check, run %d",run+1);
          } 
        if(!last_error)
          phase = BS_CHECK_PHASE_WRITE;
        break;
      case BS_CHECK_PHASE_WRITE:
        // write blocks
        if(bs == 0 && block == 0)
          MIOS32_MIDI_SendDebugMessage("Start writing blocks...");
        init_buffer();
#if BS_CHECK_USE_FRAM_LAYER == 0
        if( (last_error_code = MIOS32_IIC_BS_Write(bs,block*BS_CHECK_DATA_BLOCKSIZE,(char*)buffer,BS_CHECK_DATA_BLOCKSIZE)) != 0){
#else
        if( (last_error_code = FRAM_Write(bs,block*BS_CHECK_DATA_BLOCKSIZE,(char*)buffer,BS_CHECK_DATA_BLOCKSIZE)) != 0){
#endif
          report_iic_error();
          last_error =  BS_CHECK_ERROR_WRITE;
	  phase = BS_CHECK_PHASE_FINISHED;
          }
        else{
	  if( (last_error_code = wait_write_finished()) < 0){
	    report_iic_error();
            last_error =  BS_CHECK_ERROR_CHECK_WRITE_FINISHED;
	    phase = BS_CHECK_PHASE_FINISHED;
	    }
	  else if( ++block >= BS_CHECK_NUM_BLOCKS_PER_DEVICE ){
            MIOS32_MIDI_SendDebugMessage("Writing bankstick %d finished",bs);
	    block = 0;
	    if( ++bs >= BS_CHECK_NUM_DEVICES ){
	      bs = 0;
	      phase = BS_CHECK_PHASE_READ;
	      }
	    }
          }
        break;
      case BS_CHECK_PHASE_READ:
        // read blocks
        if(bs == 0 && block == 0)
          MIOS32_MIDI_SendDebugMessage("Start reading / comparing blocks...");
        clear_buffer();
#if BS_CHECK_USE_FRAM_LAYER == 0
        if( (last_error_code = MIOS32_IIC_BS_Read(bs,block*BS_CHECK_DATA_BLOCKSIZE,(char*)buffer,BS_CHECK_DATA_BLOCKSIZE)) != 0){
#else
        if( (last_error_code = FRAM_Read(bs,block*BS_CHECK_DATA_BLOCKSIZE,(char*)buffer,BS_CHECK_DATA_BLOCKSIZE)) != 0){
#endif
          report_iic_error();
	  last_error =  BS_CHECK_ERROR_READ;
	  phase = BS_CHECK_PHASE_FINISHED;
          }
	else{
	  if( !check_buffer() ){
	    last_error =  BS_CHECK_ERROR_COMPARE;
	    last_error_code = 0;// no further error specification
	    phase = BS_CHECK_PHASE_FINISHED;	   
	    }
	  else if( ++block >= BS_CHECK_NUM_BLOCKS_PER_DEVICE ){
            MIOS32_MIDI_SendDebugMessage("Reading / comparing bankstick %d finished",bs);
	    block = 0;
	    if( ++bs >= BS_CHECK_NUM_DEVICES ){
              MIOS32_MIDI_SendDebugMessage("Check run %d finished",run+1);
              if(++run >= BS_CHECK_NUM_RUNS)
                phase = BS_CHECK_PHASE_FINISHED;
              else{
                bs = 0;
                phase = BS_CHECK_PHASE_START;
                }
	      }
	    }
          }
        break;
      }
    }
  }


static s32 wait_write_finished(void){
#if BS_CHECK_USE_FRAM_LAYER == 0
  s32 err_code;
   do{
     err_code = MIOS32_IIC_BS_CheckWriteFinished(bs);
     } while( err_code > 0 );
  return err_code;
#else
  return 0;
#endif
  }

static void report_iic_error(void){
#if BS_CHECK_USE_FRAM_LAYER == 0
  MIOS32_MIDI_SendDebugMessage("IIC error %d",MIOS32_IIC_LastErrorGet(MIOS32_IIC_BS_PORT));
#else
  MIOS32_MIDI_SendDebugMessage("IIC error %d",MIOS32_IIC_LastErrorGet(FRAM_IIC_PORT));
#endif
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
        MIOS32_LCD_CursorSet(0,1);
        MIOS32_LCD_PrintFormattedString("Bankstick %d, Run %d",bs,run+1);
        if(msg_countdown <= 0){
          MIOS32_MIDI_SendDebugMessage("Write blocks 0x%04X (bankstick %d, run %d)",block,bs,run+1);
          msg_countdown = 0;// restart message countdown if it was disabled
          }
        break;
      case BS_CHECK_PHASE_READ:
        MIOS32_LCD_Clear();
	MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintFormattedString("Read blocks 0x%04X",block);
        MIOS32_LCD_CursorSet(0,1);
        MIOS32_LCD_PrintFormattedString("Bankstick %d, Run %d",bs,run+1);
        if(msg_countdown <= 0){
          MIOS32_MIDI_SendDebugMessage("Read blocks 0x%04X (bankstick %d, run %d)",block,bs,run+1);
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
	    MIOS32_LCD_PrintFormattedString("BS:%d B:0x%04x R:%d",bs,block,run);
	    MIOS32_MIDI_SendDebugMessage("Error on %s, Error code %d, Bankstick %d, Block %d, Run %d",
              err_on,last_error_code,bs,block,run);
	    }
	  else{
	    MIOS32_LCD_PrintFormattedString("Bankstick check");
	    MIOS32_LCD_CursorSet(0,1);	 
	    MIOS32_LCD_PrintFormattedString("success (%d runs)",BS_CHECK_NUM_RUNS);
	    MIOS32_MIDI_SendDebugMessage("Banstick check finished successfully (%d runs)!",BS_CHECK_NUM_RUNS);            
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
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ){
    run = 0;
    phase = BS_CHECK_PHASE_START;
    }
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
  if(!pin_value){
    run = 0;
    phase = BS_CHECK_PHASE_START;
    }
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
