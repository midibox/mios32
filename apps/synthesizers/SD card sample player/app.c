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

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_SAMPLES_TO_OPEN 64	// Maximum number of file handles to use, and how many samples to open
#define POLYPHONY 8				// Max voices to sound simultaneously
#define SAMPLE_SCALING 8		// Number of bits to scale samples down by in order to not distort
#define MAX_TIME_IN_DMA 40		// Time (*0.1ms) to read sample data for, e.g. 40 = 4.0 mS

#define SAMPLE_BUFFER_SIZE 512  // -> 512 L/R samples, 80 Hz refill rate (11.6~ mS period). DMA refill routine called every 5.8mS.
// NB sample rate and SPI prescaler set in mios32_config file - at 44.1kHz, reading 2 bytes per sample is SD card average rate of 86.13kB/s for a single sample

#define DEBUG_VERBOSE_LEVEL 10
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// All filenames are supported in 8.3 format

char bankprefix[13]="bank.";	// Default sample bank filename prefix on the SD card, needs to have Unix style line feeds
static file_t bank_fileinfo;	// Create the file descriptor for bank file

int sample_to_midinote[NUM_SAMPLES_TO_OPEN];		// Stores midi note mappings from bank file
char sample_filenames[NUM_SAMPLES_TO_OPEN][13];		// Stores sample mappings from bank file
u8 no_samples_loaded;								// Stores number of samples we read in from bank file (and will scan for activity)

static u32 sample_buffer[SAMPLE_BUFFER_SIZE]; // sample buffer used for DMA

static u32 samplefile_pos[NUM_SAMPLES_TO_OPEN];	// Current position in the sample file
static u32 samplefile_len[NUM_SAMPLES_TO_OPEN];	// Length of the sample file
static u8 sample_on[NUM_SAMPLES_TO_OPEN];	// To track whether each sample should be on or not
static u8 sample_vel[NUM_SAMPLES_TO_OPEN];	// Sample velocity
static file_t samplefile_fileinfo[NUM_SAMPLES_TO_OPEN];	// Create the right number of file descriptors
static u8 samplebyte_buf[POLYPHONY][SAMPLE_BUFFER_SIZE];	// Create a buffer for each voice

volatile u8 print_msg;

static u8 sdcard_access_allowed; // allow SD Card access for SYNTH_ReloadSampleBuffer

// optionally holds the sample when key released (better for drums)
#define HOLD_SAMPLE 0

// Call this routine with the sample array number to reference, and the filename to open
s32 SAMP_FILE_open(u8 sample_n, char fname[])
{
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
u32 SAMP_FILE_read(void *buffer, u32 len, u8 sample_n)
{
  s32 status;

  if( (status=FILE_ReadReOpen(&samplefile_fileinfo[sample_n])) >= 0 ) {
    status = FILE_ReadBuffer(buffer, len);
    FILE_ReadClose(&samplefile_fileinfo[sample_n]);
  }

  return (status >= 0) ? len : 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns 1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 SAMP_FILE_eof(u8 sample_n)
{
  if( samplefile_pos[sample_n] >= samplefile_len[sample_n] )
    return 1; // end of file reached

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// sets file pointer to a specific position
// returns -1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 SAMP_FILE_seek(u32 pos, u8 sample_n)	// position and the sample number
{
  s32 status;

  samplefile_pos[sample_n] = pos;

  if( samplefile_pos[sample_n] >= samplefile_len[sample_n] )
    status = -1; // end of file reached
  else {
    if( (status=FILE_ReadReOpen(&samplefile_fileinfo[sample_n])) >= 0 ) {
      status = FILE_ReadSeek(pos);    
      FILE_ReadClose(&samplefile_fileinfo[sample_n]);
    }
  }

  return status;
}

void Open_Bank(u8 b_num)	// Open the bank number passed and parse the bank information, load samples and set midi notes and number of samples
{
  u8 samp_no;
  u8 f_line[19];
  char sample_filenames[NUM_SAMPLES_TO_OPEN][13];		// Stores sample mappings from bank file
  char b_file[13];				// Overall bank name to generate
  char b_num_char[4];			// Up to 3 digit bank string plus terminator

  strcpy(b_file,bankprefix);		// Get prefix in
  sprintf(b_num_char,"%d",b_num);	// get bank number as string
  strcat(b_file,b_num_char);		// Create the final filename
  
  no_samples_loaded=0;		// Reset if it's possible to call this function on the fly
  
  DEBUG_MSG("Opening bank file %s",b_file);
  if(FILE_ReadOpen(&bank_fileinfo, b_file)<0) { DEBUG_MSG("Failed to open bank file."); }
  
  for(samp_no=0;samp_no<NUM_SAMPLES_TO_OPEN;samp_no++)	// Check for up to the defined maximum of sample mappings (one per line)
  {
	if(FILE_ReadLine(f_line, 19)) // Read line up to 19 chars long (chars 0-3=0xXX 4th=space or comma, 5th-18th is filename (8.3 format) )
	{
	   //DEBUG_MSG("Sample no %d, Line is: %s",line_no,f_line);
	   (void) strncpy(sample_filenames[samp_no],(char *)(f_line+5),13);	// Put name into array of sample names
	   sample_to_midinote[samp_no]=(int)strtol((char *)(f_line+2),NULL,16); // Convert hex string values to a real number
	   DEBUG_MSG("Sample no %d, filename is: %s, midi note value=0x%x",samp_no,sample_filenames[samp_no],sample_to_midinote[samp_no]);
	   no_samples_loaded++;	// increment global number of samples we will read in and scan for in play
	}
   }
  FILE_ReadClose(&bank_fileinfo);
    
 for(samp_no=0;samp_no<no_samples_loaded;samp_no++)	// Open all sample files and mark all samples as off
 {
  if(SAMP_FILE_open(samp_no,sample_filenames[samp_no])) { DEBUG_MSG("Open sample file failed."); }
  sample_on[samp_no]=0;
 }
}

u8 Read_Switch(void) // Lee's temp hardware: Set up inputs for bank switch as input with pullups, then read bank number (1,2,3,4) based on which of D0, D1 or D2 low
{
	 u8 pin_no;
	 u8 bank_val;

	 for(pin_no=0;pin_no<8;pin_no++)
	 {
	  MIOS32_BOARD_J10_PinInit(pin_no, MIOS32_BOARD_PIN_MODE_INPUT_PU);
	 }

	 bank_val=(u8)(MIOS32_BOARD_J10_Get() & 0x07); // Read all pins, but only care about first 3, 7=1st pos (all high), 6=2nd pos (D0 low), 5=3rd pos (D1 low), 3=4th pos (D2 low)
	 if(bank_val==3) { return 4; }
	 if(bank_val==5) { return 3; }
	 if(bank_val==6) { return 2; }	 
	 return 1;		// default to bank 1
 }

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
 // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  MIOS32_BOARD_LED_Set(0x1, 0x1);	// Turn on LED

   // print first message
  print_msg = PRINT_MSG_INIT;
  DEBUG_MSG(MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG(MIOS32_LCD_BOOT_MSG_LINE2);  
  DEBUG_MSG("Initialising SD card..");
  MIOS32_SDCARD_PowerOn();
  
  if(FILE_Init(0)<0) { DEBUG_MSG("Error initialising SD card"); } // initialise SD card

  //s32 status=FILE_PrintSDCardInfos();	// Print SD card info
  
  // Open bank file

  //Open_Bank(Read_Switch());	// For Lee's temporary bank physical switch on J10
  Open_Bank(1);	// Read Switch then Parse bank file, open sample files, set midi note mappings and no_samples_loaded

  DEBUG_MSG("Initialising synth..."); 
 // init Synth
  SYNTH_Init(0);
  DEBUG_MSG("Synth init done."); 

  MIOS32_BOARD_LED_Set(0x1, 0x0);	// Turn off LED when done with init
  MIOS32_STOPWATCH_Init(100);		// Use stopwatch in 100uS accuracy

  // allow SD Card access
  sdcard_access_allowed = 1;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  u8 samp_no;

  if( midi_package.chn == Chn1 && (midi_package.type == NoteOn || midi_package.type == NoteOff) )	// Only interested in note on/off on chn1
  {
	if( midi_package.event == NoteOn && midi_package.velocity > 0 )
	{
				for(samp_no=0;samp_no<no_samples_loaded;samp_no++)	// go through array looking for mapped notes
				{
				 if(midi_package.note==sample_to_midinote[samp_no])		// Midi note on matches a note mapped to this sample samp_no
					{
					// if(sample_on[samp_no]==2) { DEBUG_MSG("Turning sample %d on , midi note %x hex - already on",samp_no,midi_package.note); }
#if HOLD_SAMPLE
					  // if sample already on: restart it
					  if( sample_on[samp_no] ) {
					    sample_on[samp_no]=1;
						sample_vel[samp_no]=midi_package.velocity; 
					  }
#endif
						if(!sample_on[samp_no]) 
						{ 
						 sample_on[samp_no]=1; 
						 sample_vel[samp_no]=midi_package.velocity; 
}							// Mark it as want to play unless it's already on
					//DEBUG_MSG("Turning sample %d on , midi note %x hex",samp_no,midi_package.note);
					}
				}
			//}
	}
	else	// We have a note off
	{
#if !HOLD_SAMPLE
			for(samp_no=0;samp_no<no_samples_loaded;samp_no++)	// go through array looking for mapped notes
		{
		 if(midi_package.note==sample_to_midinote[samp_no])		// Midi note on matches a note mapped to this sample samp_no
			{
				sample_on[samp_no]=0;							// Mark it as off
				//DEBUG_MSG("Turning sample %d off, midi note %x hex",samp_no,midi_package.note);
			}
		}
#endif
	}
  }
  else
  {
  // DEBUG_MSG("Other MIDI message received... ignoring.");
  }
  return;
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

  if( !sdcard_access_allowed )
    return; // no access allowed by main thread

  // Each sample buffer entry contains the L/R 32 bit values
  // Each call of this routine will need to read in SAMPLE_BUFFER_SIZE/2 samples, each of which requires 16 bits
  // Therefore for mono samples, we'll need to read in SAMPLE_BUFFER_SIZE bytes

  // transfer new samples to the lower/upper sample buffer range
  int i;
  u32 *buffer = (u32 *)&sample_buffer[state ? (SAMPLE_BUFFER_SIZE/2) : 0];	// point at either 0 or the upper half of buffer
 
  s16 OutWavs16;	// 16 bit output to DAC
  s32 OutWavs32;	// 32 bit accumulator to mix samples into
  u8 samp_no;
  u8 voice;
  u8 voice_no=0;	// used to count number of voices to play
  u8 voice_samples[POLYPHONY];	// Store which sample numbers are playing in which voice
  s16 voice_velocity[POLYPHONY];	// Store the velocity for each sample
  u32 ms_so_far;		// used to measure time of DMA routine

  MIOS32_STOPWATCH_Reset();			// Reset the stopwatch at start of DMA routine
  MIOS32_BOARD_LED_Set(0x1, 0x1);	// Turn on LED at start of DMA routine
  
	// Work out which voices need to play which sample, this has lowest sample number priority
	for(samp_no=0;samp_no<no_samples_loaded;samp_no++)
	{
		if(voice_no<POLYPHONY)	// As long as not already at max voices, otherwise don't trigger/play
			{
			 if(sample_on[samp_no])	// We want to play this voice (either newly triggered =1 or continue playing =2)
				{
					voice_samples[voice_no]=samp_no;	// Assign the next available voice to this sample number
					voice_velocity[voice_no]=(s16)sample_vel[samp_no];	// Assign velocity to voice
					voice_no++;							// And increment number of voices in use
					if(sample_on[samp_no]==1)					// Newly triggered sample (set to 1 by midi receive routine)
					{
					 SAMP_FILE_seek(0,samp_no);	// make sure at start of sample
					 samplefile_pos[samp_no]=0;	// Mark at position zero (used for EOF calculations)
					 sample_on[samp_no]=2;		// Mark as on and don't retrigger on next loop
					 }
				}
			}
	}

	// Here we have voice_no samples to play simultaneously, and the samples contained in voice_samples array

	if(voice_no)	// if there's anything to play, read the samples and mix otherwise output silence
	{
		for(voice=0;voice<voice_no;voice++) 	// read up to SAMPLE_BUFFER_SIZE characters into buffer for each voice
		{
			SAMP_FILE_read(samplebyte_buf[voice],SAMPLE_BUFFER_SIZE,voice_samples[voice]); 	// Read in the appropriate number of sectors for each sample thats on

			samplefile_pos[voice_samples[voice]]+=SAMPLE_BUFFER_SIZE;	// Move along the file position by the read buffer size
			if(SAMP_FILE_eof(voice_samples[voice])) 
			{ 
				sample_on[voice_samples[voice]]=0; 
				// DEBUG_MSG("Reached EOF on sample %d",voice_samples[voice]);
			}	// if EOF don't play this sample next time and also free up the voice
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
				OutWavs16 = (s16)(OutWavs32>>SAMPLE_SCALING);	// Round down the wave to prevent distortion
				*buffer++ = (OutWavs16 << 16) | (OutWavs16 & 0xffff);	// make up the 32 bit word for L and R and write into buffer
			}
	}
	else	// There were no voices on
	 {	
	   for(i=0; i<SAMPLE_BUFFER_SIZE; i+=2) {	// Fill half the sample buffer with silence
	   *buffer++ = 0;	// Muted output
		}
	 }

	 MIOS32_BOARD_LED_Set(0x1, 0x0);	// Turn off LED at end of DMA routine
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
