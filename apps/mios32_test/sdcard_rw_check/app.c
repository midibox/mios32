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

//test-pases:
//0x00 init
//0x01 determine first rw sector
//0x02 determine last rw sector
//0x03 deep check
//0xff check done
volatile u8 phase;

volatile u32 sector;


volatile u32 bad_sector_count;
volatile u32 first_sector_rw;
volatile u32 last_sector_rw;
volatile s32 last_error_code;

//'a' (available) ; 'r' (read) ; 'w' (write) ;'c' (compare)
volatile u8 last_error_op;

volatile u16 subseq_check_errors;
u8 sdcard_buffer[0x200];

u8 was_available;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void){
  phase = 0;
  was_available = 0;
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_MIDI_SendDebugMessage("SD-Card Test init...");
  MIOS32_SDCARD_Init(0);
  // setup display task
  xTaskCreate(TASK_Display, (signed portCHAR *)"Display", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DISPLAY, NULL);
  }


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
  u32 sector_inc;//current sector increment
  MIOS32_MIDI_SendDebugMessage("SD-Card r/w check started...");
  while( 1 ) {
    switch(phase){
      case 0x02:
        sector_inc = INITIAL_SECTOR_INC;//current sector increment
        first_sector_rw = 0xffffffff;
        last_sector_rw = 0xffffffff;
        sector = 0;
        bad_sector_count = 0;
        last_error_code = 0;
        last_error_op = 0;
        subseq_check_errors = 0;
          // Connect to SD-Card
        MIOS32_MIDI_SendDebugMessage("Try to connect to SD Card...");
        if(sdcard_try_connect())
          phase = 0x03;
        break;    
      case 0x03://determine first rw sector
        if(!check_sector_rw()){
          if(sector < MAX_FIRST_SECTOR)
            sector++;
          else
            phase = 0xff;
          }
        else{
          first_sector_rw=sector;
          last_sector_rw=sector;
          phase = 0x04;
          while(sector_inc > 0xffffffff - last_sector_rw)
            sector_inc >>= 1;
          sector = last_sector_rw + sector_inc;
          }
        break;
      case 0x04://determine last rw sector
        if(!check_sector_rw()){
          if(sector_inc == 1){
            phase = 0x05;
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
      case 0x05://deep check all sectors
        if(sector > last_sector_rw){
          phase = 0xff;
          subseq_check_errors = 0;
          }
        else{
          if(!check_sector_rw()){
            bad_sector_count++;
            subseq_check_errors++;
            if(subseq_check_errors > MAX_SUBSEQ_CHECK_ERRORS){
              phase = 0xff;
              }
            }
          else
            subseq_check_errors=0;
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
  if(resp = MIOS32_SDCARD_SectorWrite(sector,sdcard_buffer)){
    last_error_op = 'w';
    last_error_code = resp;
    sdcard_try_connect();
    return 0;
    }
  clear_sdcard_buffer();
  if(resp = MIOS32_SDCARD_SectorRead(sector,sdcard_buffer)){
    last_error_op = 'r';
    last_error_code = resp;
    sdcard_try_connect();
    return 0;
    }
  if(!check_sdcard_buffer()){
    last_error_op = 'c';
    last_error_code = 0;
    return 0;
    }  
  return 1;
  }

static s32 sdcard_try_connect(){
  if(!(was_available = MIOS32_SDCARD_CheckAvailable(was_available))){
    last_error_op = 'a';
    last_error_code = 0;
    phase = 0xff;
    return  0;
    }
  return 1;
  }

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background to drive display-output
/////////////////////////////////////////////////////////////////////////////


static void TASK_Display(void *pvParameters){
  u32 delay_ms, dm_counter;
  char * op_str;
  char dm_phase;
  while(1){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    delay_ms = 100;
    switch(phase){
      case 0x00:
        MIOS32_LCD_PrintString("SD CARD R/W CHECK");
        MIOS32_LCD_CursorSet(0,1);
        MIOS32_LCD_PrintString("INITIALIZAION..");
        phase = 0x01;
        delay_ms = 1000;
        break;
      case 0x01:
        phase = 0x02;
        dm_counter = 1;
        dm_phase = 0;
        break;
      case 0x03:
        MIOS32_LCD_PrintFormattedString("FS 0x%08X",sector);
        if(!dm_counter)
          MIOS32_MIDI_SendDebugMessage("Search first w/r sector 0x%08X",sector);
        break;
      case 0x04:
        MIOS32_LCD_PrintFormattedString("LS 0x%08X",sector);
        if(!dm_counter)
          MIOS32_MIDI_SendDebugMessage("Search last w/r sector 0x%08X",sector);
        break;
      case 0x05:
        MIOS32_LCD_PrintFormattedString("0x%08X BAD %d",sector,bad_sector_count);
        MIOS32_LCD_CursorSet(0,1);
        MIOS32_LCD_PrintFormattedString("0x%08X",last_sector_rw);
        if(!dm_counter)
          MIOS32_MIDI_SendDebugMessage("0x%08X of 0x%08X sectors checked, %d bad sectors",
            sector,last_sector_rw,bad_sector_count);
        break;
      case 0xff:
        if(first_sector_rw != 0xffffffff && !subseq_check_errors){
          MIOS32_LCD_PrintFormattedString("0x%08X BAD %d",first_sector_rw,bad_sector_count);
          MIOS32_LCD_CursorSet(0,1);
          MIOS32_LCD_PrintFormattedString("0x%08X",last_sector_rw);
          if(!dm_counter && dm_phase!=phase){
            MIOS32_MIDI_SendDebugMessage("Sectors 0x%08X - 0x%08X checked, %d bad sectors",
              first_sector_rw,last_sector_rw,bad_sector_count);
            dm_phase = phase;
            }
          }
        else{
          switch(last_error_op){
            case 'r': op_str = "READ";break;
            case 'w': op_str = "WRITE";break;
            case 'a': op_str = "CONNECT";break;
            case 'c': op_str = "COMPARE";break;
            }
          MIOS32_LCD_PrintFormattedString("ABORT ON %s",op_str);
          MIOS32_LCD_CursorSet(0,1);
          MIOS32_LCD_PrintFormattedString("ERROR CODE %d",last_error_code);
          if(!dm_counter && dm_phase!=phase){
            MIOS32_MIDI_SendDebugMessage("Abort on %s, ERROR_CODE=",op_str,last_error_code);
            dm_phase = phase;
            }
          }
        break;
      }
    dm_counter = dm_counter ? dm_counter - 1 : 10;
    vTaskDelay(delay_ms / portTICK_RATE_MS);
    }
  }



/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
 
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
  if(pin_value == 0 && pin == 0)
    phase = 0x00;  
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
