// $Id: app.c 1169 2011-04-09 23:31:28Z tk $
/*
 * MIOS32 SD card polyphonic sample player
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Lee O'Donnell (lee@bassmaker.co.uk)
 *  Base software Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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
#include <file.h>
#include <string.h>

// Task stuff - the bank switch scanning is lower priority than the voice processing
#define PRIORITY_VOICE_TASK	( tskIDLE_PRIORITY + 3 )
#define PRIORITY_BANKSWITCH_TASK	( tskIDLE_PRIORITY + 2 )
static void TASK_VOICE_SCAN(void *pvParameters);
static void TASK_BANKSWITCH_SCAN(void *pvParameters);

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_SAMPLES_TO_OPEN 64	// Maximum number of file handles to use, and how many samples to open
#define POLYPHONY 8				// Max voices to sound simultaneously

// Following accounts for: 7 bits (envelope decay) + 7 bits (velocity related volume) + 1-3 bits (mixing up to 8 samples but depends how hot your samples are)
#define SAMPLE_SCALING 15        // Number of bits to scale samples down by in order to not distort - added 7 bits for midi volume now

#define MAX_TIME_IN_DMA 45		// Time (*0.1ms) to read sample data for, e.g. 40 = 4.0 mS

#define SAMPLE_BUFFER_SIZE 512  // -> 512 L/R samples, 80 Hz refill rate (11.6~ mS period). DMA refill routine called every 5.8mS.
// NB sample rate and SPI prescaler set in mios32_config file - at 44.1kHz, reading 2 bytes per sample is SD card average rate of 86.13kB/s for a single sample

#define DEBUG_VERBOSE_LEVEL 10
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// Now mandatory to have this set as legacy read code removed
#define CLUSTER_CACHE_SIZE 32 // typically for 32 * 64*512 bytes = max sample file length of 1 MB !!!

// set to 1 to perform right channel inversion for PCM1725 DAC
#define DAC_FIX 0

// name of config file on SD card to read
#define CONFIG_FNAME "player.cfg"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// All filenames are supported in 8.3 format

u8 lee_hw=0; // set to enable scanning of Lee's temporary bank switch on J10
u8 midichannel=0;	// MIDI channel to respond to

static  u8 voice_no=0;	// used to count number of voices to play
static  u8 voice_samples[POLYPHONY];	// Store which sample numbers are playing in which voice
static  s16 voice_velocity[POLYPHONY];	// Store the velocity for each sample
static s16 midi_volume=127;	// 7 bit value to scale the volume of all samples by - needs to be s16 to make multiplies work

char bankprefix[13]="bank.";	// Default sample bank filename prefix on the SD card, needs to have Unix style line feeds
static file_t bank_fileinfo;	// Create the file descriptor for bank file

int sample_to_midinote[NUM_SAMPLES_TO_OPEN];		// Stores midi note mappings from bank file
u8 no_samples_loaded;								// Stores number of samples we read in from bank file (and will scan for activity)

static u32 sample_buffer[SAMPLE_BUFFER_SIZE]; // sample buffer used for DMA

static u32 samplefile_pos[NUM_SAMPLES_TO_OPEN];	// Current position in the sample file
static u32 samplefile_len[NUM_SAMPLES_TO_OPEN];	// Length of the sample file
static s16 sample_on[NUM_SAMPLES_TO_OPEN];	// To track whether each sample should be on or not
static s8 sample_vel[NUM_SAMPLES_TO_OPEN];	// Sample velocity
static u8 sample_decay[NUM_SAMPLES_TO_OPEN];	// Sample decay parameter (used to calculate per sample, based on velocity the decrement value)
static u8 sample_decval[NUM_SAMPLES_TO_OPEN];	// Decay time read in from bank file
static u8 no_decay;								// Used to speed up decay routine if this bank has no decay time
static u8 hold_sample[NUM_SAMPLES_TO_OPEN];		// Used to hold sample (for drums)
static file_t samplefile_fileinfo[NUM_SAMPLES_TO_OPEN];	// Create the right number of file descriptors
static u8 samplebyte_buf[POLYPHONY][SAMPLE_BUFFER_SIZE];	// Create a buffer for each voice
static u32 sample_cluster_cache[NUM_SAMPLES_TO_OPEN][CLUSTER_CACHE_SIZE];	// Array of sample cluster positions on SD card

static u8 sample_bank_no=1;	// The sample bank number being played
static u8 switch_bank_no=1;	// The sample bank selected via switch for J10 
static u8 damper_pedal=0;	// Damper pedal on channel 1

volatile u8 print_msg;

static u8 sdcard_access_allowed=0; // allow SD Card access for SYNTH_ReloadSampleBuffer

// Curve to map velocity to volume of samples
static const u8 velocity_curve[128] = {
0  ,3  ,5  ,7  ,10 ,12 ,15 ,17 ,19 ,21 ,23 ,25 ,28 ,30 ,32 ,34 ,
36 ,37 ,39 ,41 ,43 ,45 ,46 ,48 ,50 ,51 ,53 ,55 ,56 ,58 ,59 ,61 ,
62 ,63 ,65 ,66 ,68 ,69 ,70 ,71 ,73 ,74 ,75 ,76 ,78 ,79 ,80 ,81 ,
82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90 ,91 ,92 ,93 ,93 ,94 ,94 ,95 ,
96 ,97 ,98 ,99 ,100,101,101,102,103,103,104,105,105,106,107,107,
108,109,109,110,110,111,111,112,112,113,113,114,114,115,115,116,
116,117,117,118,118,118,119,119,120,120,120,121,121,121,122,122,
122,123,123,123,124,124,124,125,125,125,126,126,126,127,127,127
};


// Call this routine with the sample array number to reference, and the filename to open
s32 SAMP_FILE_open(u8 sample_n, char fname[])
{
  DEBUG_MSG("Filename is %s.",fname);
  s32 status = FILE_ReadOpen(&samplefile_fileinfo[sample_n], fname);
  FILE_ReadClose(&samplefile_fileinfo[sample_n]); // close again - file will be reopened by read handler

  if( status < 0 ) {
    DEBUG_MSG("[APP] failed to open file, status: %d\n", status);
  } else {

    // got it
    samplefile_pos[sample_n] = 0;
    samplefile_len[sample_n] = samplefile_fileinfo[sample_n].fsize;

    DEBUG_MSG("[APP] Sample no %d filename %s opened of length %u\n", sample_n,fname,samplefile_len[sample_n]);
  }
  
  return status;
}

/////////////////////////////////////////////////////////////////////////////
// reads <len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
int SAMP_FILE_read(void *buffer, u32 len, u8 sample_n)
{
  // determine sector based on sample position
  u32 pos = samplefile_pos[sample_n];
  u32 sector_ix = pos / 512;
  u32 sectors_per_cluster = FILE_VolumeSectorsPerCluster();
  u32 cluster_ix = sector_ix / sectors_per_cluster;
  if( cluster_ix >= CLUSTER_CACHE_SIZE )
    return -1;

  u32 cluster = sample_cluster_cache[sample_n][cluster_ix];
  u32 phys_sector = FILE_VolumeCluster2Sector(cluster) + (sector_ix % sectors_per_cluster);
  if( MIOS32_SDCARD_SectorRead(phys_sector, buffer) < 0 )
    return -2;
  return len;
}

void Open_Bank(u8 b_num)	// Open the bank number passed and parse the bank information, load samples, set midi notes, number of samples and cache cluster positions
{
  u8 samp_no;
  u8 f_line[63];				// 0..3=0xXX (hex midi note) 4=space 5=sample hold (0 or 1) 6=space 7..10=decay (4 digit decimal) 11=space 12..23=8.3 filename 24=null
  char b_file[13];				// Overall bank name to generate
  char b_num_char[4];			// Up to 3 digit bank string plus terminator
  static char sample_filenames[NUM_SAMPLES_TO_OPEN][30];		// Stores sample mappings from bank file, needs to be static to avoid crash

  midi_volume=127;		// Reset midi volume to default value
  
  strcpy(b_file,bankprefix);		// Get prefix in
  sprintf(b_num_char,"%d",b_num);	// get bank number as string
  strcat(b_file,b_num_char);		// Create the final filename
  
  MIOS32_BOARD_LED_Set(0x1, 0x1);	// Turn on LED during bank load
  
  no_samples_loaded=0;
  no_decay=1;						// Default to no decay for bank
  
  DEBUG_MSG("Opening bank file %s",b_file);
  if(FILE_ReadOpen(&bank_fileinfo, b_file)<0) { DEBUG_MSG("Failed to open bank file."); }
   else
  {
		  for(samp_no=0;samp_no<NUM_SAMPLES_TO_OPEN;samp_no++)	// Check for up to the defined maximum of sample mappings (one per line)
		  {
			if(FILE_ReadLine(f_line, 63)) // Read line up to 63 chars long
			{
			   //DEBUG_MSG("Sample no %d, Line is: %s",samp_no,f_line);
			   sample_to_midinote[samp_no]=(int)strtol((char *)(f_line+2),NULL,16); // Convert hex string values to a real number (pos 2 on line, base 16)
			   hold_sample[samp_no]=(int)strtol((char *)(f_line+5),NULL,10); // Convert sample hold digit
			   sample_decval[samp_no]=(int)strtol((char *)(f_line+7),NULL,10); // Convert decay number (pos 5 on line, base 10)
			   if(sample_decval[samp_no]>0) { no_decay=0; }	// At least one of the samples requires decay processing
			   (void) strncpy(sample_filenames[samp_no],(char *)(f_line+12),30);	// Put name into array of sample names (pos 10 on line), up to 12 chars (8.3)   
			   DEBUG_MSG("Sample no %d, filename is: %s, midi note value=0x%x, decay value %d, hold=%d",samp_no,sample_filenames[samp_no],sample_to_midinote[samp_no],sample_decval[samp_no],hold_sample[samp_no]);
			   no_samples_loaded++;	// increment global number of samples we will read in and scan for in play
			}
		   }
		  FILE_ReadClose(&bank_fileinfo);
			
		 for(samp_no=0;samp_no<no_samples_loaded;samp_no++)	// Open all sample files and mark all samples as off
		 {
		   if(SAMP_FILE_open(samp_no,sample_filenames[samp_no])) {
		   DEBUG_MSG("Open sample file failed.");
		   } else {
			 // Pre-read all the cluster positions for all samples to open
			 u32 num_sectors_per_cluster = FILE_VolumeSectorsPerCluster();
			 u32 cluster_ix;
			 for(cluster_ix=0; cluster_ix < CLUSTER_CACHE_SIZE; ++cluster_ix) {
			   u32 pos = cluster_ix*num_sectors_per_cluster*SAMPLE_BUFFER_SIZE;

			   if( pos >= samplefile_len[samp_no] )
			 break; // end of file reached
			   else {
			 s32 status;
			 if( (status=FILE_ReadReOpen(&samplefile_fileinfo[samp_no])) >= 0 ) {
			   status = FILE_ReadSeek(pos);
			   if( status >= 0 ) {
				 u8 dummy; // dummy read to update cluster
				 status = FILE_ReadBuffer(&dummy, 1);
			   }
			   FILE_ReadClose(&samplefile_fileinfo[samp_no]);
			 }
			 if( status < 0 )
			   break;
			   }

			   sample_cluster_cache[samp_no][cluster_ix] = samplefile_fileinfo[samp_no].curr_clust;
			   DEBUG_MSG("Cluster %d: %d ", cluster_ix, sample_cluster_cache[samp_no][cluster_ix]);
			 }
		   }

		   sample_on[samp_no]=0;	// Set sample to off
		 }
	}
	MIOS32_BOARD_LED_Set(0x1, 0x0);	// Turn off LED after bank load
}

u8 Read_Switch(void) // Lee's temp hardware: Set up inputs for bank switch as input with pullups, and find if any lines pulled low to select bank 
{
     u8 pin_no;
     u8 bank_val;

     for(pin_no=0;pin_no<8;pin_no++)
     {
      MIOS32_BOARD_J10_PinInit(pin_no, MIOS32_BOARD_PIN_MODE_INPUT_PU);
     }

     bank_val=(u8)MIOS32_BOARD_J10_Get(); // Read all pins, if all pins high, val=0 meaning bank 1, otherwise one pin should be pulled low eg bank_index 0 = bank_val=1 so bank 2
	 if(bank_val==127) { return 9; }	// D7 = 128 low
	 if(bank_val==191) { return 8; }	// D6 = 64 low
	 if(bank_val==223) { return 7; }	// D5 = 32 low
	 if(bank_val==239) { return 6; }	// D4 = 16 low
	 if(bank_val==247) { return 5; }	// D3 = 8 low
	 if(bank_val==251) { return 4; }	// D2 = 4 low
	 if(bank_val==253) { return 3; }	// D1 = 2 low
	 if(bank_val==254) { return 2; }	// D0 = 1 low
     return 1;      // default to bank 1 (bank val 255)
 }

void Read_Config()	// Open the config file on the SD card and (re)set various settings for the player
{
  u8 f_line[25];					// parameter and value space separated
  char *param_name, *param_value;	//
  static file_t config_fileinfo;	// Create the file descriptor for config file
  
  DEBUG_MSG("Opening config file %s",CONFIG_FNAME);
  if(FILE_ReadOpen(&config_fileinfo, CONFIG_FNAME)<0) { DEBUG_MSG("Failed to open config file."); return; }
  
  while(1)	// keep going until we get an EOF
  {
	if(FILE_ReadLine(f_line, 25)) // Read line up to 24 chars long
	{
	   //DEBUG_MSG("Config Line is: %s",f_line);
	   param_name=strtok((char *)f_line, " ");	// find param name up to the space
	   param_value=strtok(NULL,"\n");	// find value
	   DEBUG_MSG("Found param %s and value %s",param_name,param_value);
	   if(!strcmp(param_name,"lee_hw")) { lee_hw=(int)strtol((char *)(param_value),NULL,10); } // Set lee_hw param
	   if(!strcmp(param_name,"midichannel")) { midichannel=((int)strtol((char *)(param_value),NULL,10)-1); } // Set midichannel (midi receive routines need -1 from decimal value)
	}
	else { break; }	// Hit EOF
   }
  FILE_ReadClose(&config_fileinfo);
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
 // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

   // print first message
  print_msg = PRINT_MSG_INIT;
  DEBUG_MSG(MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG(MIOS32_LCD_BOOT_MSG_LINE2);  
  DEBUG_MSG("Initialising SD card..");
  
  if(FILE_Init(0)<0) { DEBUG_MSG("Error initialising SD card"); } // initialise SD card

  // wait until SD Card available
  int timeout_ctr;
  for(timeout_ctr=0; timeout_ctr<1000; ++timeout_ctr)
    if( MIOS32_SDCARD_CheckAvailable(0) >= 1 )
      break;

  if( timeout_ctr == 1000 ) {
    DEBUG_MSG("SD Card error: connection failed!");
  }

  //s32 status=FILE_PrintSDCardInfos();	// Print SD card info

  // Read config file from SD card
  Read_Config();
  
  // Open bank file

  if (lee_hw)	// If we have non standard bank switch connected
  {
	  DEBUG_MSG("Reading J10 switch");
	  sample_bank_no=Read_Switch();	// For Lee's temporary bank physical switch on J10 - read first bank to load on boot
  }
  
  Open_Bank(sample_bank_no);	// Open default bank on boot (1 if Lee's switch not read)
 
  DEBUG_MSG("Initialising synth..."); 

  // allow SD Card access
  sdcard_access_allowed = 1;  

  // init Synth
  SYNTH_Init(0);
  DEBUG_MSG("Synth init done."); 

  MIOS32_STOPWATCH_Init(100);		// Use stopwatch in 100uS accuracy
  
  // Start tasks for voice processing and bank switch scanning
  xTaskCreate(TASK_VOICE_SCAN, (signed portCHAR *)"VOICE_SCAN", configMINIMAL_STACK_SIZE, NULL, PRIORITY_VOICE_TASK, NULL);
  xTaskCreate(TASK_BANKSWITCH_SCAN, (signed portCHAR *)"BANKSWITCH_SCAN", configMINIMAL_STACK_SIZE, NULL, PRIORITY_BANKSWITCH_TASK, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  u8 samp_no;

  if( midi_package.chn == midichannel && (midi_package.type == NoteOn || midi_package.type == NoteOff) )	// Only interested in note on/off on chn1
  {
	if( midi_package.event == NoteOn && midi_package.velocity > 0 )
	{
				for(samp_no=0;samp_no<no_samples_loaded;samp_no++)	// go through array looking for mapped notes
				{
				 if(midi_package.note==sample_to_midinote[samp_no])		// Midi note on matches a note mapped to this sample samp_no
					{
						 sample_on[samp_no]=-1; 	// Retrigger the sample
						 sample_vel[samp_no]=velocity_curve[midi_package.velocity]; 
							// Mark it as want to play unless it's already on
					//DEBUG_MSG("Turning sample %d on , midi note %x hex",samp_no,midi_package.note);
					}
				}
			//}
	}
	else	// We have a note off
	{
		for(samp_no=0;samp_no<no_samples_loaded;samp_no++)	// go through array looking for mapped notes
		{
			if (!hold_sample[samp_no] && !damper_pedal)	// if not holding sample or damper pedal not pressed, turn the note off, otherwise do nothing
			{
				if(midi_package.note==sample_to_midinote[samp_no])		// Midi note on matches a note mapped to this sample samp_no
				{
					if(no_decay || sample_decval[samp_no]==0 || sample_on[samp_no]==0) { sample_on[samp_no]=0; }		// Turn off immediately if no decay for bank or this sample or the sample finished
					else
					{
						sample_on[samp_no]=sample_decval[samp_no];							// Mark it as decaying with the appropriate time for this sample
						sample_decay[samp_no]=1+sample_vel[samp_no]/((sample_decval[samp_no]>>3)+1);		// Amount to decay by each 8th time around the DMA routine (big = fast decay)
					}
					//DEBUG_MSG("Turning sample %d off, midi note %x hex",samp_no,midi_package.note);
				}
			}
		}
	}
  }
  else if (midi_package.chn==midichannel && midi_package.type==ProgramChange)
    {
		sample_bank_no=midi_package.evnt1;	// Set new bank
		DEBUG_MSG("MIDI Program Change received - Changing bank to %d",sample_bank_no);
		sdcard_access_allowed=0;
		DEBUG_MSG("Opening new sample bank");
		Open_Bank(sample_bank_no);	// Load relevant bank
		sdcard_access_allowed=1;
	}
  else if (midi_package.chn==midichannel && midi_package.type==CC && midi_package.evnt1==7) // Volume message
  {
   midi_volume=midi_package.evnt2;  // Set new global midi volume
   //DEBUG_MSG("Midi volume set to %d\n", midi_volume);
  }
  else
  {
  //DEBUG_MSG("Other MIDI message received, channel %d, event %X, type %X, cc number %d, value %d... ignoring.",midi_package.chn,midi_package.event,midi_package.type,midi_package.cc_number,midi_package.value);
  }
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
}

/////////////////////////////////////////////////////////////////////////////
// This function is called by MIOS32_I2S when the lower (state == 0) or 
// upper (state == 1) range of the sample buffer has been transfered, so 
// that it can be updated
/////////////////////////////////////////////////////////////////////////////
void SYNTH_ReloadSampleBuffer(u32 state)
{
  // transfer new samples to the lower/upper sample buffer range
  int i;
  u32 *buffer = (u32 *)&sample_buffer[state ? (SAMPLE_BUFFER_SIZE/2) : 0];	// point at either 0 or the upper half of buffer

  if( !sdcard_access_allowed )	// no access allowed by main thread
    {
	 for(i=0; i<SAMPLE_BUFFER_SIZE; i+=2) {	// Fill half the sample buffer with silence
	   *buffer++ = 0;	// Muted output
	 }
	 return; 	 
	}

  // Each sample buffer entry contains the L/R 32 bit values
  // Each call of this routine will need to read in SAMPLE_BUFFER_SIZE/2 samples, each of which requires 16 bits
  // Therefore for mono samples, we'll need to read in SAMPLE_BUFFER_SIZE bytes

  u8 voice;

  s16 OutWavs16;	// 16 bit output to DAC
  s32 OutWavs32;	// 32 bit accumulator to mix samples into
  u32 ms_so_far;		// used to measure time of DMA routine

  MIOS32_STOPWATCH_Reset();			// Reset the stopwatch at start of DMA routine
  MIOS32_BOARD_LED_Set(0x1, 0x1);	// Turn on LED at start of DMA routine
  

	// Here we have voice_no samples to play simultaneously, and the samples contained in voice_samples array

	if(voice_no)	// if there's anything to play, read the samples and mix otherwise output silence
	{
		for(voice=0;voice<voice_no;voice++) 	// read up to SAMPLE_BUFFER_SIZE characters into buffer for each voice
		{
			if(SAMP_FILE_read(samplebyte_buf[voice],SAMPLE_BUFFER_SIZE,voice_samples[voice])<0) // Read in the appropriate number of sectors for each sample thats on
			{	// if <0 then there was an error reading, so turn this sample off and mute it
			 sample_on[voice_samples[voice]]=0; // Turn sample off
			 voice_velocity[voice]=0;			// Silence it in the mix as we don't have a complete buffer
			}
			
			samplefile_pos[voice_samples[voice]]+=SAMPLE_BUFFER_SIZE;	// Move along the file position by the read buffer size
			if(samplefile_pos[voice_samples[voice]] >= samplefile_len[voice_samples[voice]]) // We've reached EOF - don't play this sample next time and also free up the voice
			{ 
				sample_on[voice_samples[voice]]=0; // Turn sample off
				//DEBUG_MSG("Reached EOF on sample %d",voice_samples[voice]);
			}
			 ms_so_far= MIOS32_STOPWATCH_ValueGet();	// Check how long we've been in the routine up until this point
			if(ms_so_far>MAX_TIME_IN_DMA)				
				{ 
				 DEBUG_MSG("Abandoning DMA routine after %d.%d ms, after %d voices out of %d",ms_so_far/10,ms_so_far%10,voice+1,voice_no);
				 voice_no=voice+1;	// don't mix un-read voices (eg if break after 1st voice, voice_no should =1)
				 break;	// go straight to sample mixing
				}
		}

		for(i=0; i<SAMPLE_BUFFER_SIZE; i+=2) // Fill half the sample buffer
			{	
				OutWavs32=0;	// zero the voice accumulator for this sample output
				for(voice=0;voice<voice_no;voice++)
				{
						OutWavs32+=voice_velocity[voice]*(s16)((samplebyte_buf[voice][i+1] << 8) + samplebyte_buf[voice][i]);		// else mix it in
				}
				OutWavs32 = (OutWavs32>>SAMPLE_SCALING);	// Round down the wave to prevent distortion, and factor in the velocity multiply
				if(OutWavs32>32767) { OutWavs32=32767; }	// Saturate positive
				if(OutWavs32<-32768) { OutWavs32=-32768; }	// Saturate negative			
				OutWavs16 = (s16)OutWavs32;					// Required to make following bit shift work correctly including sign bit
#if DAC_FIX
				*buffer++ = (OutWavs16 << 16) | (-OutWavs16 & 0xffff);	// make up the 32 bit word for L and R and write into buffer with fix for PCM1725 DAC
#else
				*buffer++ = (OutWavs16 << 16) | (OutWavs16 & 0xffff);	// make up the 32 bit word for L and R and write into buffer
#endif
				}
	}
	else	// There were no voices on
	 {	
	   for(i=0; i<SAMPLE_BUFFER_SIZE; i+=2) {	// Fill half the sample buffer with silence
	   *buffer++ = 0;	// Muted output
		}
	 }

	 MIOS32_BOARD_LED_Set(0x1, 0x0);	// Turn off LED at end of DMA routine
	//ms_so_far= MIOS32_STOPWATCH_ValueGet();	// Check how long we've been in the routine up until this point
	//DEBUG_MSG("%d.%d ms,%d.%d ms,%d.%d ms, %d voices",ms2_so_far/10,ms2_so_far%10,ms3_so_far/10,ms3_so_far%10,ms_so_far/10,ms_so_far%10,voice_no);
}

/////////////////////////////////////////////////////////////////////////////
// initializes the synth
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_Init(u32 mode)
{
  // start I2S DMA transfers
  return MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &SYNTH_ReloadSampleBuffer);
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
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

static void TASK_VOICE_SCAN(void *pvParameters)
{
  u8 samp_no;
  u8 new_voice_no;
  
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) 
  {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);		// Run this every 1 ms, this WILL be interrupted every now and again by the DMA fill interrupt

		// toggle Status LED to as a sign of live
		//MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

		new_voice_no=0;
		
		// Work out which voices need to play which sample, this has lowest sample number priority
		for(samp_no=0;samp_no<no_samples_loaded;samp_no++)
		{
			if(new_voice_no<POLYPHONY)	// As long as not already at max voices, otherwise don't trigger/play
				{
				 if(sample_on[samp_no]<0)	// We want to play this voice (either newly triggered =1 or continue playing =2)
					{
						voice_samples[new_voice_no]=samp_no;	// Assign the next available voice to this sample number
						//voice_velocity[new_voice_no]=(s16)(sample_vel[samp_no]);	// Assign velocity to voice - cast required to ensure the voice accumulation multiply is fast signed 16 bit
						voice_velocity[new_voice_no]=(s16)(sample_vel[samp_no]*midi_volume);    // Assign velocity to voice - cast required to ensure the voice accumulation multiply is fast signed 16 bit
						new_voice_no++;							// And increment number of voices in use
						if(sample_on[samp_no]==-1)					// Newly triggered sample (set to -1 by midi receive routine)
						{
						 samplefile_pos[samp_no]=0;	// Mark at position zero (used for sector reads and EOF calculations)
						 sample_on[samp_no]=-2;		// Mark as on and don't retrigger on next loop
						 }
					}
				
				}
				else
				{ break; }		// stop looking if we're full!
		}

		if(!no_decay || new_voice_no<POLYPHONY)	// Only process decaying notes if decay is enabled or we have any voices left
		{
			// now new notes allocated, fill up any remaining voices with decaying ones
			for(samp_no=0;samp_no<no_samples_loaded;samp_no++)
			{
				if(new_voice_no<POLYPHONY)	// As long as not already at max voices, otherwise don't trigger/play
					{
					if(sample_on[samp_no]>0)	// positive number = decaying
						{
							voice_samples[new_voice_no]=samp_no;	// Assign the next available voice to this sample number
							voice_velocity[new_voice_no]=(s16)(sample_vel[samp_no]*midi_volume);					
							new_voice_no++;							// And increment number of voices in use				
							sample_on[samp_no]--;				// Decrement decay time
							if(sample_on[samp_no]<0) { sample_vel[samp_no]=0; sample_on[samp_no]=0;}	// If finished decaying mark as off
							else
							{
								if((sample_on[samp_no]%8)==0) { 
									sample_vel[samp_no]-=sample_decay[samp_no];		// decrement volume by appropriate amount 
									//DEBUG_MSG("vel is %d, sample on is %d, sample_decay is %d",sample_vel[samp_no],sample_on[samp_no],sample_decay[samp_no]);
								   }
							}
							if(sample_vel[samp_no]<=0) { sample_vel[samp_no]=0; sample_on[samp_no]=0; }
						}
					
					}
					else
					{ break;}	// stop looking if we're full
			}
		}

	voice_no=new_voice_no;	// Set the global voice count now we're done
	
	}
}

static void TASK_BANKSWITCH_SCAN(void *pvParameters)
{
 u8 this_bank;
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) 
  {
	vTaskDelayUntil(&xLastExecutionTime, 100 / portTICK_RATE_MS); // Run this every 0.1s, this WILL be interrupted every now and again by the DMA fill interrupt
	if(lee_hw)	// non standard bank switch connected and enabled in config file
	{	
			// Now check for bank switch change  
			this_bank=Read_Switch();	// Get bank value
			if(this_bank!=switch_bank_no) { 
				switch_bank_no=this_bank;	// Set new bank to compare on switch - this is now separate to not interfere with MIDI program changes
				sample_bank_no=this_bank;	// Set new bank
				DEBUG_MSG("Changing bank to %d",sample_bank_no);
				sdcard_access_allowed=0;
				DEBUG_MSG("Opening new sample bank");
				Open_Bank(sample_bank_no);	// Load relevant bank
				sdcard_access_allowed=1;
			}
	}
  }
}
