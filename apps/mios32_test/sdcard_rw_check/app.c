/*
 * Checks a connected SD-Card (read / write & compare )
 * RUNING THIS APPLICATION WILL DESTROY ALL DATA ON YOUR SD-CARD!!!
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Matthias Mächler (thismaechler@gmx.ch / maechler@mm-computing.ch)
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
// Local definitions
/////////////////////////////////////////////////////////////////////////////


#define PRIORITY_TASK_DISPLAY  ( tskIDLE_PRIORITY + 1 )

#define MAX_FIRST_SECTOR 0xFF
#define MAX_SUBSEQ_CHECK_ERRORS  0x10
#define CHECK_STEP 0xF2
#define INITIAL_SECTOR_INC 0x1000

// display messages
#define DISPLAY_TASK_DELAYMS 100
#define DISPLAY_TASK_MSG_COUNTDOWN_STARTVALUE 10


// test phases
#define SDCARD_CHECK_PHASE_STARTWAIT 1
#define SDCARD_CHECK_PHASE_START 2
#define SDCARD_CHECK_PHASE_INIT 3
#define SDCARD_CHECK_PHASE_FIND_FIRSTSECTOR 4
#define SDCARD_CHECK_PHASE_FIND_LASTSECTOR 5
#define SDCARD_CHECK_PHASE_DEEPCHECK 6
#define SDCARD_CHECK_PHASE_FINISHED 16

// errors
#define SDCARD_CHECK_ERROR_INIT 1
#define SDCARD_CHECK_ERROR_CONNECT 2
#define SDCARD_CHECK_ERROR_READ 3
#define SDCARD_CHECK_ERROR_WRITE 4
#define SDCARD_CHECK_ERROR_COMPARE 5

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_Display(void *pvParameters);

static void init_sdcard_buffer(void);
static s32 check_sdcard_buffer(void);
static void clear_sdcard_buffer(void);
static s32 check_sector_rw(void);
static s32 sdcard_try_connect();

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 phase;

volatile u32 sector;


volatile u32 bad_sector_count;
volatile u32 first_sector_rw;
volatile u32 last_sector_rw;
volatile s32 last_error_code;

volatile u8 last_error;

volatile u16 subseq_check_errors;
u8 sdcard_buffer[0x200];

u8 was_available;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void){
  was_available = 0;
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_MIDI_SendDebugMessage("SD-Card r/w check");
  MIOS32_MIDI_SendDebugMessage("Init driver...");
  last_error_code = 0;
  last_error = 0;
  if( (last_error_code = MIOS32_SDCARD_Init(0)) < 0){
    last_error = SDCARD_CHECK_ERROR_INIT;
    phase = SDCARD_CHECK_PHASE_FINISHED;
    }
  else{
    phase = SDCARD_CHECK_PHASE_STARTWAIT;
    }
  // setup display task
  xTaskCreate(TASK_Display, (signed portCHAR *)"Display", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DISPLAY, NULL);
  }


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
  u32 sector_inc;//current sector increment
  while( 1 ) {
    switch(phase){
      case SDCARD_CHECK_PHASE_INIT:
        sector_inc = INITIAL_SECTOR_INC;//current sector increment
        first_sector_rw = 0xffffffff;
        last_sector_rw = 0xffffffff;
        sector = 0;
        bad_sector_count = 0;
        last_error_code = 0;
        last_error = 0;
        subseq_check_errors = 0;
	// Connect to SD-Card
        MIOS32_MIDI_SendDebugMessage("Try to connect to SD Card...");
        if(sdcard_try_connect())
          phase = SDCARD_CHECK_PHASE_FIND_FIRSTSECTOR;
        break;    
      case SDCARD_CHECK_PHASE_FIND_FIRSTSECTOR://determine first rw sector
        if(!check_sector_rw()){
          if(sector < MAX_FIRST_SECTOR)
            sector++;
          else
            phase = SDCARD_CHECK_PHASE_FINISHED;
          }
        else{
          first_sector_rw=sector;
          last_sector_rw=sector;
          phase = SDCARD_CHECK_PHASE_FIND_LASTSECTOR;
          while(sector_inc > 0xffffffff - last_sector_rw)
            sector_inc >>= 1;
          sector = last_sector_rw + sector_inc;
          }
        break;
      case SDCARD_CHECK_PHASE_FIND_LASTSECTOR://determine last rw sector
        if(!check_sector_rw()){
          if(sector_inc == 1){
            phase = SDCARD_CHECK_PHASE_DEEPCHECK;
            subseq_check_errors = 0;
            sector = first_sector_rw;
            }
          else{
            sector_inc >>= 1;
            sector = last_sector_rw + sector_inc;
            }
          }
        else{
          last_sector_rw = sector;
          while(sector_inc > 0xffffffff - last_sector_rw)
            sector_inc >>= 1;
          sector += sector_inc;
          }
        break;
      case SDCARD_CHECK_PHASE_DEEPCHECK://deep check all sectors
        if(sector > last_sector_rw){
          phase = SDCARD_CHECK_PHASE_FINISHED;
          subseq_check_errors = 0;
          }
        else{
          if(!check_sector_rw()){
            bad_sector_count++;
            if(subseq_check_errors > MAX_SUBSEQ_CHECK_ERRORS){
              phase = SDCARD_CHECK_PHASE_FINISHED;
              }
            }
          if(phase != SDCARD_CHECK_PHASE_FINISHED)
            sector += CHECK_STEP;
          }
        break;
      }
    }
  }

static void init_sdcard_buffer(void){
  u16 i;
  u8 value;
   value = sector % 0xff;
  for(i = 0; i < 0x200;i++){
    sdcard_buffer[i] = value;
    value = (value < 0xff) ? value + 1 : 0;
    }
  }

static s32 check_sdcard_buffer(void){
  u16 i;
  u8 value;
   value = sector % 0xff;
  for(i = 0; i < 0x200;i++){
    if(sdcard_buffer[i] != value)
      return 0;
    value = (value < 0xff) ? value + 1 : 0;
    }
  return 1;
  }
  
static void clear_sdcard_buffer(void){
  u16 i;
  for(i = 0; i < 0x200;i++)
    sdcard_buffer[i] = 0;
  }

static s32 check_sector_rw(void){
  s32 resp;
  init_sdcard_buffer();
  subseq_check_errors++;
  if(resp = MIOS32_SDCARD_SectorWrite(sector,(u8*)sdcard_buffer)){
    last_error = SDCARD_CHECK_ERROR_WRITE;
    last_error_code = resp;
    sdcard_try_connect();
    return 0;
    }
  clear_sdcard_buffer();
  if(resp = MIOS32_SDCARD_SectorRead(sector,(u8*)sdcard_buffer)){
    last_error = SDCARD_CHECK_ERROR_READ;
    last_error_code = resp;
    sdcard_try_connect();
    return 0;
    }
  if(!check_sdcard_buffer()){
    last_error = SDCARD_CHECK_ERROR_COMPARE;
    last_error_code = 0;
    return 0;
    }  
  subseq_check_errors = 0;
  return 1;
  }

static s32 sdcard_try_connect(){
  if(!(was_available = MIOS32_SDCARD_CheckAvailable(was_available))){
    last_error = SDCARD_CHECK_ERROR_CONNECT;
    last_error_code = 0;
    phase = SDCARD_CHECK_PHASE_FINISHED;
    return  0;
    }
  return 1;
  }

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background to drive display-output
/////////////////////////////////////////////////////////////////////////////


static void TASK_Display(void *pvParameters){
  char * op_str;
  s32 msg_countdown = 0;
  while(1){
    switch(phase){
      case SDCARD_CHECK_PHASE_STARTWAIT:
        if(msg_countdown != -phase){
	  MIOS32_LCD_Clear();
	  MIOS32_LCD_CursorSet(0,0);
	  MIOS32_LCD_PrintString("SD-Card r/w check");
	  MIOS32_LCD_CursorSet(0,1);
	  MIOS32_LCD_PrintString("Push a button/key..");
          MIOS32_MIDI_SendDebugMessage("Push any button or MIDI-key to start the check..");
          msg_countdown = -phase;
          }
        break;
      case SDCARD_CHECK_PHASE_START:
        msg_countdown = -phase;
        phase = SDCARD_CHECK_PHASE_INIT;
        break;
      case SDCARD_CHECK_PHASE_FIND_FIRSTSECTOR:
        MIOS32_LCD_Clear();
	MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintFormattedString("FS 0x%08X",sector);
        if(msg_countdown <= 0){
          MIOS32_MIDI_SendDebugMessage("Search first w/r sector 0x%08X",sector);
          msg_countdown = 0;// restart message countdown if it was disabled
          }
        break;
      case SDCARD_CHECK_PHASE_FIND_LASTSECTOR:
        MIOS32_LCD_Clear();
	MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintFormattedString("LS 0x%08X",sector);
        if(msg_countdown <= 0){
          MIOS32_MIDI_SendDebugMessage("Search last w/r sector 0x%08X",sector);
          msg_countdown = 0;// restart the message countdown if it was disabled
          }
        break;
      case SDCARD_CHECK_PHASE_DEEPCHECK:
        MIOS32_LCD_Clear();
	MIOS32_LCD_CursorSet(0,0);
        MIOS32_LCD_PrintFormattedString("0x%08X BAD %d",sector,bad_sector_count);
        MIOS32_LCD_CursorSet(0,1);
        MIOS32_LCD_PrintFormattedString("0x%08X",last_sector_rw);
        if(msg_countdown <= 0){
          MIOS32_MIDI_SendDebugMessage("0x%08X of 0x%08X sectors checked, %d bad sectors",
            sector,last_sector_rw,bad_sector_count);
          msg_countdown = 0;// restart message countdown if it was disabled
          }
        break;
      case SDCARD_CHECK_PHASE_FINISHED:
        if(msg_countdown != -phase){// was this stuff already printed/ sent do debug condsole?
          msg_countdown = -phase;// disable further message output in this phase
          MIOS32_LCD_Clear();
	  MIOS32_LCD_CursorSet(0,0);
	  if(first_sector_rw != 0xffffffff && !subseq_check_errors){
	    MIOS32_LCD_PrintFormattedString("0x%08X BAD %d",first_sector_rw,bad_sector_count);
	    MIOS32_LCD_CursorSet(0,1);
	    MIOS32_LCD_PrintFormattedString("0x%08X",last_sector_rw);
	    MIOS32_MIDI_SendDebugMessage("Sectors 0x%08X - 0x%08X checked, %d bad sectors",
	      first_sector_rw,last_sector_rw,bad_sector_count);
	    }
	  else{
	    switch(last_error){
	      case SDCARD_CHECK_ERROR_INIT: op_str = "init";break;
	      case SDCARD_CHECK_ERROR_READ: op_str = "read";break;
	      case SDCARD_CHECK_ERROR_WRITE: op_str = "write";break;
	      case SDCARD_CHECK_ERROR_CONNECT: op_str = "connect";break;
	      case SDCARD_CHECK_ERROR_COMPARE: op_str = "compare";break;
	      }
	    MIOS32_LCD_PrintFormattedString("Abort on %s",op_str);
	    MIOS32_LCD_CursorSet(0,1);
	    MIOS32_LCD_PrintFormattedString("Error code %d",last_error_code);
	    MIOS32_MIDI_SendDebugMessage("Abort on %s, error code: %d",op_str,last_error_code);
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
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( midi_package.type == NoteOn && midi_package.velocity > 0 )
    phase = SDCARD_CHECK_PHASE_START;  
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
  if(pin_value == 0)
    phase = SDCARD_CHECK_PHASE_START;  
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
