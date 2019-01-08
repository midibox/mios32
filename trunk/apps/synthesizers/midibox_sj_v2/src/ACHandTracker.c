/*
 *  ACHandTracker.c
 *  kII.3
 *
 *  Created by audiocommander on 14.12.06.
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
 *
 */




#include <mios32.h>
#include "app.h"
#include "ACHandTracker.h"
#include "IIC_Speakjet.h"


#if KII_AIN_ENABLED



// ***** GLOBALS *****

volatile ht_state_t		ht_state;				// handTracker settings
volatile ht_t			ht;						// handTracker status vars

volatile unsigned char	lastAinPin;				// last active pin
volatile unsigned char	lastValue[SENSOR_NUM];	// last 7bit values (per sensor)
volatile unsigned char	lastOpen[(LAST_QUEUE_MAX + 1)];	// last open state, to detect gestures

// ***** LOCALS *****

volatile unsigned char	lastRoll;				// last roll value
volatile unsigned char  lastSpeed;				// last speed value
volatile unsigned char	lastEnunciate;			// last enunciation type, eg SJCH_CONSONANTS_OPEN
volatile unsigned char	lastPhonemeType;		// contains last spoken phoneme type, eg PHONEME_VOWEL
volatile unsigned char	lastPhoneme;			// contains last spoken phoneme, eg MSAPH_AX

volatile unsigned char	sensor_slowdown_counter[SENSOR_NUM];
#if SYNC_ENABLED == 0
volatile unsigned char	gesture_slowdown_counter;
#endif /* SYNC_ENABLED == 0 */

#if USE_COMPLEX_LINEARIZING == 0
// log to lin lookup table; make the sensor reading linear and therefore detection easier
#define LIN_DISTANCE_TABLE_COUNT	68
const unsigned char		linearDistance[LIN_DISTANCE_TABLE_COUNT] = {
	0,	0,	0,	0,	0,	1,	2,	3,	4,	5,	5,	6,		// 0   -  11
	6,	7,	7,	8,	8,	9,	9,	10,	10,	11,	11,	12,		// 12  -  23
	12,	12,	13,	13,	13,	14,	14,	14,	14,	15,	15,	15,		// 24  -  35
	15,	16,	16,	16,	16,	16,	17,	17,	17,	17,	17,	18,		// 36  -  47
	18,	18,	18,	18,	18,	19,	19,	19,	19,	19,	19,	19,		// 48  -  59
	20,	20,	20,	20,	20,	20,	20,	20						// 60  -  67
};
#endif /* USE_COMPLEX_LINEARIZING == 0 */


// ***** MACROS *****

// returns MAXIMUM value
#define ACMAX(a,b) ( (a >= b) ? a : b )
// returns MINIMUM value
#define ACMIN(a,b) ( (a <= b) ? a : b )
// returns DIFFERENCE value (always positive), eg. ACDIFF(10,12) = 2  or ACDIFF(12,10) = 2
#define ACDIFF(a,b)  ( (a <= b) ? (b - a) : (a - b) )



/////////////////////////////////////////////////////////////////////////////
//	This function initializes the handtracker
/////////////////////////////////////////////////////////////////////////////

void ACHandTracker_Init(void) {
	unsigned char c;
	
	//#error "TK: AIN initialisation: TODO! - 4 pins have to be configured!"
	
	// setup sensors vars
	for(c=0; c<SENSOR_NUM; c++) {
		// init vars
		lastValue[c] = 20;
		sensor_slowdown_counter[c] = 0;
	}

	// setup gesture detection vars
#if SYNC_ENABLED == 0
	gesture_slowdown_counter = 0;
#endif /* SYNC_ENABLED == 0 */
	ht_state.ALL = 0;
	ht_state.GESTURE_UPDATE_REQ = TRUE;
	// init lastOpen
	for(c=0; c<=LAST_QUEUE_MAX; c++) {
		lastOpen[c] = 0;
	}
}



/////////////////////////////////////////////////////////////////////////////
//	This function recalculates all handtracker values
//  and detects gestures (triggers phonemes)
/////////////////////////////////////////////////////////////////////////////

void ACHandTracker_Update(void) {
	unsigned char c;						// local incrementer for lastOpen
	unsigned char changeAmount = 1;			// determine quality of movement
	signed	 char tempValue;				// to calculate ht.OPEN & ht.HEIGHT
	unsigned char speed;					// determine speed of movement
	unsigned char speedMax;					
	unsigned char speedMin;
	unsigned char bend;						// determine voice bending (relaxed/stressed)
	unsigned char enunciate = 0;			// to enunciate ht.OPEN
	
	// ***** filter *****
	if( ! ht_state.GESTURE_UPDATE_REQ ) {	// no AIN changes, no update req
		return; 
	}
	if( SPEAKJET_GET_D2_PIN() ) { return; } // no processing if buffer already half full!
	
	// ***** clear state vars *****
	ht_state.ALL = 0x0;
	
#if SYNC_ENABLED == 0
#ifndef _DEBUG_C
	// check if slowdown value reached (slowdown detection)
	if(GESTURE_SLOWDOWN) {
		gesture_slowdown_counter++;
		if(gesture_slowdown_counter >= GESTURE_SLOWDOWN) {
			gesture_slowdown_counter = 0;
		} else {
			return;
		}
	}
#endif /* _DEBUG_C */
#endif /* SYNC_ENABLED == 0 */
	
	// ***** update finger *****
	ht.FINGER = ACMIN(lastValue[HT_TL], lastValue[HT_TR]);				// 20..0
	// ***** update thumb *****
	ht.THUMB = ACMIN(lastValue[HT_BL], lastValue[HT_BR]);				// 20..0
	
	// ***** abort if finger & thumb both on minimum (no hand in there!)
#if KIII_MODE
	if( (ht.FINGER > 20) && (ht.THUMB > 20) ) { 
		ACHandTracker_Mute();
		return;
	}
#else
	if( (ht.FINGER > 10) && (ht.THUMB > 10) ) { 
		ACHandTracker_Mute();
		return;
	}
#endif /* KIII_MODE */
	
	// ***** update height *****
	tempValue = 20 + ht.FINGER - ht.FINGER - ht.FINGER;					// 0..20
	tempValue = (tempValue << 2) + 15;									// 15..95
	ht.HEIGHT = (unsigned char)tempValue;
	tempValue = 0;
#if HARMONIZER_ENABLED
	// harmonize height
	ht.HEIGHT = ACHarmonize(ht.HEIGHT);
#endif /* HARMONIZER_ENABLED */
	
	// ***** update jaw (open) *****
	// calculate open (virtual third beam); see openForumula demo example!
	tempValue = 14 - ht.FINGER - ht.THUMB;								// 0..14
	if(tempValue < 0) {
		tempValue = 0;
	} else if(tempValue > 14) {
		tempValue = 14;
	}
	ht.OPEN = (unsigned char)tempValue;
	// set jaw
	jaw = ht.OPEN;
	// track & update last open states (open history)
	for(c=LAST_QUEUE_MAX;c>0;c--) {
		lastOpen[c] = lastOpen[(c-1)];
	}
	lastOpen[0] = ht.OPEN;
	
	// ***** rapid change? *****
	changeAmount = ACDIFF(lastOpen[0], lastOpen[1]);
	
	// ***** update roll *****
	if(ht.OPEN > 0) {
		// update roll: roll = DIFF(TopLeft, TopRight)
		lastRoll = ht.ROLL;
		tempValue = lastValue[HT_TR] - lastValue[HT_TL];				// -20..+20 (theoretically)
		if(tempValue < -2) {
			ht.ROLL = 0;
		} else if(tempValue > 3) {
			ht.ROLL = 4;
		} else {														// 0..4
			switch(tempValue) {
				case -2: case -1:		ht.ROLL = 1;	break;
				case  0: case  1:		ht.ROLL = 2;	break;
				case  2: case  3:		ht.ROLL = 3;	break;
			}
		}
		// set tongue
		tongue = ht.ROLL;
	}
	
	// ***** update speed & bend *****
	if(ht.OPEN > 2) {
		speedMax = ACMAX(lastOpen[0],lastOpen[1]);
		speedMin = ACMIN(lastOpen[0],lastOpen[1]);
		speed = speedMax - speedMin;								// 0..20 (theoretically)
		// ** update bend **
		bend = ACDIFF(lastValue[HT_BL], lastValue[HT_BR]);			// 0..20
		if((bend < 2) || (bend > 15)) {
			ht.BEND = 5;
		} else {
			ht.BEND = bend;
		}
		// ** update speed **
		speed = (speed << 1) + 80;									// 80..120
		if(speed > 127) { speed = 127; }
		if(speed < 80)  { speed = 80; }								// 80..127
		ht.SPEED = speed;
		if(ht.SPEED != lastSpeed) {
			ht_state.UPDATE_SPEED = TRUE;
			ht_state.UPDATE_BEND = TRUE; 
		}
		lastSpeed = ht.SPEED;
	}

	
	// ***** detect gesture *****
	while(ht_state.UPDATE_GESTURE == FALSE) {
		
		// mouth is closed
		if((lastOpen[0] == 0) && (lastOpen[1] == 0)) {
			ACHandTracker_Mute();
			return;									// abort process
		}
		
		// mouth is opening
		if((lastOpen[0] > 1) && (lastOpen[0] > lastOpen[1]) && (lastOpen[1] > lastOpen[2])) {
			// trigger vowels & diphtongs OR consonants
			if(lastPhonemeType == PHONEME_CONSONANT) {
				enunciate = SJCH_VOWELS_DIPHTONGS;
			} else {
				enunciate = SJCH_CONSONANTS_OPEN;
			}
			ht_state.UPDATE_GESTURE = TRUE;			// abort gesture detection
			break;
		}
		
		// mouth is closing
		if((lastOpen[0] < 2) && (lastOpen[0] < lastOpen[1]) && (lastOpen[1] < lastOpen[2])) {
			// trigger vowels & diphtongs OR consonants
			if(lastPhonemeType == PHONEME_CONSONANT) {
				enunciate = SJCH_VOWELS_DIPHTONGS;
			} else {
				enunciate = SJCH_CONSONANTS_CLOSE;
			}
			ht_state.UPDATE_GESTURE = TRUE;			// abort gesture detection
			break;
		}
		
		// mouth is open
		if(lastOpen[0] > 1) {
			if(changeAmount < 2) {					// ----- small hand change -----
				switch(lastPhonemeType) {
					case PHONEME_VOWEL:				// keep the same vowel
						enunciate = SMEAR_VOWEL;
						break;
					case PHONEME_DIPHTONG:			// trigger vowel
						enunciate = SJCH_VOWELS;
						break;
					case PHONEME_CONSONANT:			// trigger vowel or diphtong
						enunciate = SJCH_VOWELS;
						break;
					case PHONEME_NONE:
						enunciate = SJCH_CONSONANTS;
						break;
				}
			} else {
				if(changeAmount > 3) {				// ----- huge hand change -----
					IIC_SPEAKJET_ClearBuffer();		// clear pending messages
					enunciate = SJCH_CONSONANTS;	// trigger consonant
				} else {		// ----- normal hand change -----
					switch(lastPhonemeType) {
						case PHONEME_VOWEL:
						case PHONEME_DIPHTONG:
						case PHONEME_NONE:			// trigger consonant
							enunciate = SJCH_CONSONANTS;
							break;
						case PHONEME_CONSONANT:
							enunciate = SJCH_VOWELS;
							break;
					}
				}
			}
			ht_state.UPDATE_GESTURE = TRUE;			// abort gesture detection
			break;
		}
		
		ht_state.UPDATE_GESTURE = TRUE;				// leave while loop
	}
	
		
#ifdef _DEBUG_C
	// for debug output reasons it is important that lastPhonemeType is stored before
	// sending the phonemes via IIC_SPEAKJET_Transmit()
	// while on the chip the storage is less important/time critical and is therefore
	// called _after_ sending the messages (see below)
	lastEnunciate = enunciate;
	switch(enunciate) {
		case SJCH_VOWELS:
			lastPhonemeType = PHONEME_VOWEL;
			break;
		case SJCH_VOWELS_DIPHTONGS:
		case SJCH_DIPHTONGS:
			lastPhonemeType = PHONEME_DIPHTONG;
			break;
		case SJCH_CONSONANTS:
		case SJCH_CONSONANTS_OPEN:
		case SJCH_CONSONANTS_CLOSE:
			lastPhonemeType = PHONEME_CONSONANT;
			break;
		default:
			lastPhonemeType = PHONEME_NONE;
			break;
	}
#endif /* _DEBUG_C */
	
	// ***** trigger phoneme *****
	if(enunciate) {
		// enter manualTransmission mode
		// we have to send at least 1, max 6 IIC messages
		// this way we're saving 12 Start/Stop messages to enable much smoother output
		// that lowers chances of scrambled pings/beeps/shots that may occur elsewise from time to time
		IIC_SPEAKJET_TransmitStart(0);
		sj_control.manualTransmission = TRUE;		// already sent start ignoring next starts	
		// set speed
		if(ht_state.UPDATE_SPEED) {
			// we got the check ht.SPEED again, there's something cheesy about it around...
			if(ht.SPEED < 80) { ht.SPEED = 80; };
			if(ht.SPEED > 127){ ht.SPEED = 127;};
			IIC_SPEAKJET_MSA_Speed( ht.SPEED );		// set speed
			if(ht.SPEED > 120) {					// set volume (based on speed)
				IIC_SPEAKJET_MSA_Volume(127);
			} else {
				IIC_SPEAKJET_MSA_Volume(122);
			}	
		}
		// set bend
		if(ht_state.UPDATE_BEND) { 
			// check bend for range before sending
			if(ht.BEND > 15) { ht.BEND = 15; };
			IIC_SPEAKJET_Direct_Bend(ht.BEND); 
		}
		// enunciate
		if(enunciate == SMEAR_VOWEL) {
//			IIC_SPEAKJET_ClearBuffer();				// clear pending messages; disabled because of gaps
			IIC_SPEAKJET_MSA_Pitch(ht.HEIGHT,FALSE);	
			IIC_SPEAKJET_MSA_Ctrl(lastPhoneme);
		} else {
			lastPhoneme = IIC_SPEAKJET_Enunciate(enunciate,ht.HEIGHT);
		}
#if EXTENDED_HT_MODE
		switch(channelAssignment + 1) {				// channelAssignment: 0..15; SJCH_xx: 1..16
/*			case SJCH_SOUNDCODES:					// we'll leave it like it is ;)
			case SJCH_ALLOPHONES: case SJCH_FX: 
			case SJCH_VOWELS: case SJCH_VOWELS_DIPHTONGS: 
				break;
*/			case SJCH_VOWELS_CONSONANTS: 
			case SJCH_CONSONANTS: case SJCH_CONSONANTS_OPEN: case SJCH_CONSONANTS_CLOSE:
			case SJCH_PERCUSSIVE:					// insert short pauses
				IIC_SPEAKJET_MSA_Pauses(20);		// 10 ms
				break;
			case SJCH_PITCH:						// alter Envelope
				IIC_SPEAKJET_ENVFreq(ht.HEIGHT);
				break;								// change OSC value
			case SJCH_OSC1: case SJCH_OSC2: case SJCH_OSC3: case SJCH_OSC4: case SJCH_OSC5:
				// (channelAssignment) for OSCs should be 10,11,12,13,14
				c = channelAssignment - 9;			// 
				IIC_SPEAKJET_OSCFreq(c,ht.HEIGHT);
				IIC_SPEAKJET_OSCLvl(c,ht.SPEED);
				break;
			case SJCH_OSC_SYN:
				c = ACMath_RandomInRange(5);
				IIC_SPEAKJET_OSCFreq(c,ht.HEIGHT);
				IIC_SPEAKJET_OSCLvl(c,ht.SPEED);
				break;
		}
#endif /* EXTENDED_HT_MODE */
		// cleanup (leave manualTransmission mode)
		sj_control.manualTransmission = FALSE;		// switch back transmissionMode
		IIC_SPEAKJET_TransmitStop();
	}
	
#ifndef _DEBUG_C
	// store last phoneme/enunciation
	lastEnunciate = enunciate;
	switch(enunciate) {
		case SJCH_VOWELS:
			lastPhonemeType = PHONEME_VOWEL;
			break;
		case SJCH_VOWELS_DIPHTONGS:
		case SJCH_DIPHTONGS:
			lastPhonemeType = PHONEME_DIPHTONG;
			break;
		case SJCH_CONSONANTS:
		case SJCH_CONSONANTS_OPEN:
		case SJCH_CONSONANTS_CLOSE:
			lastPhonemeType = PHONEME_CONSONANT;
			break;
		default:
			lastPhonemeType = PHONEME_NONE;
			break;
	}
#endif /* _DEBUG_C */

#if HANDTRACKER_CC_OUTPUT
	if(enunciate) {
		// send NOTE_ON
#if HANDTRACKER_NOTE_VELOCITY == 0
	  MIOS32_MIDI_SendNoteOn(DEFAULT, channelAssignment, ht.HEIGHT, (lastPhoneme - 127)); // Note: 15..95
#else
	  MIOS32_MIDI_SendNoteOn(DEFAULT, channelAssignment, ht.HEIGHT, HANDTRACKER_NOTE_VELOCITY); // Note: 15..95
#endif /* HANDTRACKER_NOTE_VELOCITY */
		// send jaw (ht.OPEN)
	        MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, SJCC_MOUTH_JAW, (jaw << 3)); // 0..14 => 0..112
		// send tongue (ht.THUMB)
	        MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, SJCC_MOUTH_TONGUE, (tongue << 4)); // 0..4 => 0..64
		// send bend
	        MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, SJCC_MOUTH_STRESS, (ht.BEND << 3)); // 0..4 => 0..64
		// send speed
	        MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, SJCC_MOUTH_SPEED, (ht.SPEED)); // 80..127
#if HANDTRACKER_CC_OUTPUT_VERBOSE || KIII_MODE
		// send AIN
		MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, (MIDI_CC_SENSOR_A + lastAinPin), lastValue[lastAinPin]);
		// send height
		MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, SJCC_MOUTH_HEIGHT, ht.HEIGHT); // 15..95
		// send phoneme
		MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, SJCC_MOUTH_PHONEME, (lastPhoneme - 127) ); // 0..127
#endif /* HANDTRACKER_CC_OUTPUT_VERBOSE */	
	}	
#endif /* HANDTRACKER_CC_OUTPUT */

}


/////////////////////////////////////////////////////////////////////////////
//	This function simply mutes the SJ if no hand or a closing one is detected
/////////////////////////////////////////////////////////////////////////////

void ACHandTracker_Mute(void) {
	unsigned char c;
	if(lastPhonemeType != PHONEME_NONE) {
		// mute pending sounds
		IIC_SPEAKJET_Reset(FALSE);
		lastPhonemeType = PHONEME_NONE;
		lastEnunciate = 0;
		// track & update last open states (open history)
		ht.OPEN = 0;
		for(c=LAST_QUEUE_MAX;c>0;c--) {
			lastOpen[c] = lastOpen[(c-1)];
		}
		lastOpen[0] = ht.OPEN;
#if HANDTRACKER_CC_OUTPUT
		// send jaw (ht.OPEN)
		MIOS32_MIDI_SendCC(DEFAULT, channelAssignment, SJCC_MOUTH_JAW, 0);
#endif /* HANDTRACKER_CC_OUTPUT */
	}
}


/////////////////////////////////////////////////////////////////////////////
//	This function gets notified when an AIN change is received
//	The Sensor values get updated. If the Sync option is disabled the
//	gesture detector gets polled. The GP2D120 values get linearized if
//  USE_COMPLEX_LINEARIZE is enabled (see below for more informations)
/////////////////////////////////////////////////////////////////////////////

void ACHandTracker_AIN_NotifyChange(unsigned char pin, unsigned int pin_value) {
	// hand tracker AIN notify
	unsigned int tempValue;
	unsigned char value = 0;
	
	// sensorize
	if(pin < SENSOR_NUM) {
#ifndef _DEBUG_C
		// abort if slowdown active
		// process anyway if value > SMIN to abort wrong readings immediatly
		// see it as improved releaseDetect or gate in the lower field with higher resolution
		if( SENSOR_SLOWDOWN && (pin_value > SMIN) ) {
			sensor_slowdown_counter[pin]++;
			if(sensor_slowdown_counter[pin] >= SENSOR_SLOWDOWN) { 
				sensor_slowdown_counter[pin] = 0;	// protect against overflow
			} else {
				return;  	// abort if slowdown does not match
			}
		}
#endif /* _DEBUG_C */
		
		// abort if pin value below or above min/max
		if(pin_value <= SMIN) {
			// release detect
			if(lastValue[pin] != 0) { 
				pin_value = 0;
			} else {
				return;
			}
		} else if(pin_value > SMAX) {
			return;
		}
		
#if USE_COMPLEX_LINEARIZING
		// the following math is described here:
		// http://www.acroname.com/robotics/info/articles/irlinear/irlinear.html
#if HT_SENSORTYPE == GP2D120X
		// the formula for the GP2D120's linearization (4-30cm) is:
		// R = (2914 / (V + 5)) - 1
		// get the distance in cm: 3 to 23 cm => 655 to 117 [10bit]
		tempValue = ACMath_Divide( COMPLEX_LINEAR_K, (pin_value + 5) );	 		 // 4..24 cm
		tempValue = tempValue - 4;								// -1: 3..23 => -4: 0..20 cm
#else /* HT_SENSORTYPE == GP2D12J0 */
		// this has never been tested; it's probably better to use these sensors with the same k than GP2D120X
		// because all calculations are optimized for 0..26 cm; as the voltage is relative, this should be no problem...
		// the formula for the GP2D12's linearization (10-80cm) is:
		// R = (6787 / (V - 3)) - 4
		tempValue = ACMath_Divide( COMPLEX_LINEAR_K, (pin_value - 3) );			// 10..80 cm
		tempValue = tempValue - 13;								// -4: 9..80 => -13: 0..70 cm
		tempValue = tempValue >> 1;								// 0..35
#endif /* HT_SENSORTYPE */
		value = (unsigned char)tempValue;
#else
		// restrict value from 0..1024 (=> 117..655) to 0..67
		tempValue = pin_value + SMIN;							// ie: 655-117 = 538
		tempValue = tempValue >> 3;								// ie: 538 >>3 = 67
		value = linearDistance[tempValue];						// 0..20
#endif /* USE_COMPLEX_LINEARIZING */
			
		// filter results (important! see tongue & jaw @gestureDetection!)
		if(value > 20) { value = 20; }

		// store last value
		lastValue[pin] = value;
		// save last pin for #CC output
		lastAinPin = pin;
		// update request for gesture detection
		ht_state.GESTURE_UPDATE_REQ = TRUE;
		
#ifdef _DEBUG_C
//		printf("\n***MIOS_AIN_Notify, pin[#%i]: %i",pin,value);
#endif /* _DEBUG_C */
		
#if SYNC_ENABLED == 0
		if(value > TRACKER_THRESHOLD) {
			// update all values incl. gesture detection
			ACHandTracker_Update();
		}
#endif /* SYNC_ENABLED == 0 */
		
	}
}


#endif /* KII_AIN_ENABLED */

