// $Id: app.c 346 2009-02-10 22:10:39Z tk $
/*
 * Demo application for DMX Controller.
 * It used a DOG GLCD but could be adapted for any LCD.
 * It has 2 banks of 12 faders (BANK A and BANKB)
 * Plus 2 master faders (MASTER A and MASTER B
 * It is currently just a simple 12 channel desk
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
#include <math.h>
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <semphr.h>

#include "app.h"
#include "dmx.h"
#include "fs.h"
#include <glcd_font.h>
#include <uwindows.h>
#include <uip.h>
#include <uip_arp.h>
#include <timer.h>
#include "dhcpc.h"
#include "osc_client.h"

// Mutex for controlling access to SDCARD/ENC28J60
xSemaphoreHandle xSPI0Semaphore;	

#define PRIORITY_TASK_CHASE               ( tskIDLE_PRIORITY + 4 ) // lowest priority
#define DEBUG_VERBOSE_LEVEL 0
static UW_Window	FaderForm;
static UW_Window	SubmasterForm;
static UW_Window	ConfigForm;
static UW_Window    LabelName,LabelHome,LabelMob,LabelEmail;
static UW_Window	EditName,EditHome,EditMob,EditEmail;
static UW_Window	BtnOk,BtnCancel;


static UW_Window	LightsBtn,LightsLabel;
static UW_Window	Faders[26];
static UW_Window	Submasters[26];
static const char IconCaption[]="   MIDIbox Lights";
static const char FaderCaption[]="   Faders View";
static const char SubmasterCaption[]="Submaster View";
static const char ConfigCaption[]="SSSSSTTTTT";
static char LabelCaption[] = "Hello UWindows!";
static char BtnCaption[]= "Show/Hide";

static void TASK_CHASE(void *pvParameters);
const char Labels[]="AccountName:Home:Mob:Email:MaleSingleOkCancel";
u8 banka[12];
u8 bankb[12];
u8 sub[24];
u8 mastera, masterb;
u8 current_state;
u8 uip_configured=0;
static struct timer periodic_timer,arp_timer;

// Initialize all views
void Views_Init(void)
{
	u8 f;
	/////////////////////////////////////////////////////////
	//FADER_VIEW
	/////////////////////////////////////////////////////////
	UW_Add(&FaderForm, UW_FORM, &UWindows, NULL);
	/* Set Form	caption */
	UW_SetCaption(&FaderForm, FaderCaption, 22);
	/* Display the Form in Main Window	*/
	UW_SetFont(&FaderForm, GLCD_FONT_NORMAL);
	UW_SetVisible(&FaderForm, 1);
	for (f=0;f<12;f++) {
		UW_Add(&Faders[f],UW_FADER, &FaderForm, NULL);
		UW_SetPosition(&Faders[f], (f*10)+10,50);
		UW_Add(&Faders[f+12],UW_FADER, &FaderForm, NULL);
		UW_SetPosition(&Faders[f+12], (f*10)+10,10);
	}
	// Master faders
	UW_Add(&Faders[24],UW_FADER, &FaderForm, NULL);
	UW_Add(&Faders[25],UW_FADER, &FaderForm, NULL);
	UW_SetPosition(&Faders[24], 140,30);
	UW_SetPosition(&Faders[25], 150,30);
	// Get current fader values
	for (f=0;f<26;f++)
		UW_SetValue(&Faders[f],MIOS32_AIN_PinGet(f)>>4); 
	/////////////////////////////////////////////////////////
	//SUBMASTER_VIEW
	/////////////////////////////////////////////////////////
	UW_Add(&SubmasterForm, UW_FORM, &UWindows, NULL);
	/* Set Form	caption */
	UW_SetCaption(&SubmasterForm, SubmasterCaption, 22);
	/* Hide the Form in Main Window	*/
	UW_SetVisible(&SubmasterForm, 0);
	for (f=0;f<12;f++) {
		UW_Add(&Submasters[f],UW_FADER, &SubmasterForm, NULL);
		UW_SetPosition(&Submasters[f], (f*10)+10,50);
		UW_Add(&Submasters[f+12],UW_FADER, &SubmasterForm, NULL);
		UW_SetPosition(&Submasters[f+12], (f*10)+10,10);
	}
	// Master faders
	UW_Add(&Submasters[24],UW_FADER, &SubmasterForm, NULL);
	UW_Add(&Submasters[25],UW_FADER, &SubmasterForm, NULL);
	UW_SetPosition(&Submasters[24], 140,30);
	UW_SetPosition(&Submasters[25], 150,30);
	// Get current fader values
	for (f=0;f<26;f++)
		UW_SetValue(&Submasters[f],0); 
		
	/////////////////////////////////////////////////////////
	//CONFIG_VIEW
	/////////////////////////////////////////////////////////
	UW_Add(&ConfigForm, UW_FORM, &UWindows, NULL);
	/* Set Form	caption */
	UW_SetCaption(&ConfigForm, ConfigCaption, 10);
	/* Hide the Form in Main Window	*/
	UW_SetVisible(&ConfigForm, 0);
	UW_SetFont(&ConfigForm, GLCD_FONT_BIG);
	
}



/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
	MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs
	current_state=FADER_VIEW;
	UW_Init();
	DMX_Init(0);
	Views_Init();
	// MUST be initialized before the SPI functions
	xSPI0Semaphore = xSemaphoreCreateMutex();
  
	FS_Init(0);
	// start uIP task
	UIP_TASK_Init(0);

	xTaskCreate(TASK_CHASE, "CHASE", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_CHASE, NULL);
}

void Check_Faders(void)
{
	u8 f;
	for (f=0;f<24;f++)
	{
		if (f<12) {
			if (Faders[f].value!=banka[f]) {
				banka[f]=Faders[f].value;
				int q=(((banka[f]*mastera)/255)|((bankb[f]*(u8)~masterb)/255));
				DMX_SetChannel(f,(u8)q);
				//MIOS32_MIDI_SendDebugMessage("Channel: %d = %d (Fader: %d = %d)\n",f,(u8)q,f,banka[f]);
			}
		} else if (f>=12){
			if (Faders[f].value!=bankb[f-12]) {
				bankb[f-12]=Faders[f].value;
				int q=(((banka[f-12]*mastera)/255)|((bankb[f-12]*(u8)~masterb)/255));
				DMX_SetChannel(f-12,(u8)q);
				//MIOS32_MIDI_SendDebugMessage("Channel: %d = %d (Fader: %d = %d)\n",f,(u8)q,f,bankb[f-12]);
			}
		}
	}
}

void Check_Submasters(void)
{
	u8 f;
	for (f=0;f<24;f++)
	{
		if (f<12) {
			if (Faders[f].value!=banka[f]) {
				banka[f]=Faders[f].value;
				int q=(((banka[f]*mastera)>>8)|((bankb[f]*(u8)~masterb)>>8));
				DMX_SetChannel(f,(u8)q);
				//MIOS32_MIDI_SendDebugMessage("Channel: %d = %d\n",f,(u8)q);
			}
		} else if (f>=12){
			if (Faders[f].value!=bankb[f-12]) {
				bankb[f-12]=Faders[f].value;
				int q=(((banka[f-12]*mastera)>>8)|((bankb[f-12]*(u8)~masterb)>>8));
				DMX_SetChannel(f-12,(u8)q);
				//MIOS32_MIDI_SendDebugMessage("Channel: %d = %d\n",f-12,(u8)q);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
	u32 g;
	u8 temp_mastera, temp_masterb;
	
	// endless loop
	while( 1 )
	{
		UW_Run();	// Do any outstanding window function.
		if (current_state==FADER_VIEW) {
			Check_Faders(); // Scan all of the faders and update the DMX value if faders have moved.
			temp_mastera=Faders[24].value;
			temp_masterb=Faders[25].value;
		} else if (current_state==SUBMASTER_VIEW) {
			Check_Submasters(); // Scan all of the faders and update the DMX value if faders have moved.
			temp_mastera=Submasters[24].value;
			temp_masterb=Submasters[25].value;
		}
		// Master faders are always live!
		if ((temp_mastera!=mastera) || (temp_masterb!=masterb)) {
			// If either of the master faders have moved, we must recalculate the whole universe.
			mastera=temp_mastera;
			masterb=temp_masterb;
			MIOS32_MIDI_SendDebugMessage("Master A = %d  Master B = %d\n",mastera,(u8)~masterb);
			for(g=0; g<12; g++) {
				u8 q=(((banka[g]*mastera)>>8)|((bankb[g]*(u8)~masterb)>>8));
				if (DMX_GetChannel(g) != q)
					DMX_SetChannel(g,(u8)q);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
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
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
	u32 temp_state=current_state;
	if (pin_value==0) { // Button Pressed	
		if (pin==0)
			current_state--;
		else if (pin==1)
			current_state++;
		switch (current_state) {
			case FADER_VIEW:
				UW_SetVisible(&ConfigForm, 0);
				UW_SetVisible(&SubmasterForm, 0);
				UW_SetVisible(&FaderForm, 1);
				break;
			case SUBMASTER_VIEW:
				UW_SetVisible(&FaderForm, 0);
				UW_SetVisible(&ConfigForm, 0);
				UW_SetVisible(&SubmasterForm, 1);
				break;
			case CONFIG_VIEW:
				UW_SetVisible(&FaderForm, 0);
				UW_SetVisible(&ConfigForm, 1);
				UW_SetVisible(&SubmasterForm, 0);
				break;
			default:
				current_state=temp_state;
		}
	}
	UW_Keypress(pin, pin_value); 
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
	pin=((pin%4)<<3)+(pin>>2);	// change muxed pin order.
	
		pin_value=pin_value>>4;
	if (current_state==FADER_VIEW) {
		if (pin<26)
			UW_SetValue(&Faders[pin],pin_value);
	} else if (current_state==SUBMASTER_VIEW) {
		if (pin<26)
			UW_SetValue(&Submasters[pin],pin_value);
	}

	if (uip_configured) // UIP Is active and working!
	{
#if DEBUG_VERBOSE_LEVEL >= 1
		MIOS32_MIDI_SendDebugMessage("[AIN] %d %d\n", pin, pin_value);
#endif
		// send AIN value as OSC message
		mios32_osc_timetag_t timetag;
		timetag.seconds = 0;
		timetag.fraction = 1;
		OSC_CLIENT_SendPotValue(timetag, pin, (float)pin_value / 255.0);

	}

}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS to handle chases
/////////////////////////////////////////////////////////////////////////////
static void TASK_CHASE(void *pvParameters)
{
  portTickType xLastExecutionTime;
  u8 sdcard_available = 0;
  int sdcard_check_ctr = 0;
  MIOS32_MIDI_SendDebugMessage("About to initialize SDCard Reader\n");

  // initialize SD Card
  MIOS32_SDCARD_Init(0);

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

#if 1

    // each second: check if SD card (still) available
    if( ++sdcard_check_ctr >= 1000 ) {
      sdcard_check_ctr = 0;

      // check if SD card is available
      // High-speed access if SD card was previously available
      u8 prev_sdcard_available = sdcard_available;
      sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

      if( sdcard_available && !prev_sdcard_available ) {
        MIOS32_MIDI_SendDebugMessage("SD Card has been connected!\n");

        s32 status;
        if( (status=FS_mount_fs()) < 0 ) {
          MIOS32_MIDI_SendDebugMessage("File system cannot be mounted, status: %d\n", status);
        } //else {

        // }
      } else if( !sdcard_available && prev_sdcard_available ) {
        MIOS32_MIDI_SendDebugMessage("SD Card has been disconnected!\n");

      }
    }
#endif

  }
}


void APP_MutexSPI0Take(void)
{
	// This must block as we aren't telling the calling process that the semaphore couldn't be obtained!
	while( xSemaphoreTake(xSPI0Semaphore, (portTickType)1) != pdTRUE ); 
	//MIOS32_MIDI_SendDebugMessage("Semaphore taken\n");


	/*s32 ret=pdFALSE;
	while (ret!=pdTRUE) {	
	if (xSemaphoreTake(xSPI0Semaphore, (portTickType)portTICK_RATE_MS*1000) != pdTRUE);
		MIOS32_MIDI_SendDebugMessage("Semaphore (SPI0) not released in 1 second *HELP*\n");
	}*/
	
	return;	
}


void APP_MutexSPI0Give(void)
{
	xSemaphoreGive(xSPI0Semaphore);
	//MIOS32_MIDI_SendDebugMessage("Semaphore given\n");

	return;	
}


void dhcpc_configured(const struct dhcpc_state *s)
{	
	MIOS32_MIDI_SendDebugMessage("Got IP address %d.%d.%d.%d\n",
		uip_ipaddr1(s->ipaddr), uip_ipaddr2(s->ipaddr),
		uip_ipaddr3(s->ipaddr), uip_ipaddr4(s->ipaddr));
	MIOS32_MIDI_SendDebugMessage("Got netmask %d.%d.%d.%d\n",
		uip_ipaddr1(s->netmask), uip_ipaddr2(s->netmask),
		uip_ipaddr3(s->netmask), uip_ipaddr4(s->netmask));
    MIOS32_MIDI_SendDebugMessage("Got DNS server %d.%d.%d.%d\n",
		uip_ipaddr1(s->dnsaddr), uip_ipaddr2(s->dnsaddr),
		uip_ipaddr3(s->dnsaddr), uip_ipaddr4(s->dnsaddr));
    MIOS32_MIDI_SendDebugMessage("Got default router %d.%d.%d.%d\n",
		uip_ipaddr1(s->default_router), uip_ipaddr2(s->default_router),
		uip_ipaddr3(s->default_router), uip_ipaddr4(s->default_router));
    MIOS32_MIDI_SendDebugMessage("Lease expires in %d hours\n",
		(ntohs(s->lease_time[0])*65536ul + ntohs(s->lease_time[1]))/3600);

	
	uip_sethostaddr(s->ipaddr);
	uip_setnetmask(s->netmask);
	uip_setdraddr(s->default_router);
	  // start telnet daemon
	telnetd_init(); //(now we have an IP address)
    // start OSC daemon and client
    OSC_CLIENT_Init(0);
    OSC_SERVER_Init(0);
	uip_configured=1;
}
