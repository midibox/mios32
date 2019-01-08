/*
 *  IIC_SpeakJetDefines.h
 *  kII
 *
 *  Created by Michael Markert, audiocommander.de on 20.05.06
 *  Based on Speakjet control codes from July 27, 2004 version of Speakjet Manual
 *   and ASM-Version of Doug Elliott, VA3DAE
 *
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
 *
 */

/*
 * Released under GNU General Public License
 * http://www.gnu.org/licenses/gpl.html
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation
 *
 * YOU ARE ALLOWED TO COPY AND CHANGE 
 * BUT YOU MUST RELEASE THE SOURCE TOO (UNDER GNU GPL) IF YOU RELEASE YOUR PRODUCT 
 * YOU ARE NOT ALLOWED TO USE IT WITHIN PROPRIETARY CLOSED-SOURCE PROJECTS
 */


#ifndef _IIC_SPEAKJETDEFINES_H
#define _IIC_SPEAKJETDEFINES_H



// ********* SPEAKJET SCP ************* //
// Serial Control Protocol
#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark SCP
#endif

// escape character ('\\')	0x5C to enter SCP Mode
#define SCP_ESCAPE				'\\'
#define SCP_SEL0				'0'
#define SCP_SEL1				'1'
#define SCP_SEL2				'2'
#define SCP_SEL3				'3'
#define SCP_SEL4				'4'
#define SCP_SEL5				'5'
#define SCP_SEL6				'6'
#define SCP_SEL7				'7'
#define SCP_EXIT				'X'

#define SCP_READY				'V'
#define SCP_CLEAR_BUFFER		'R'
#define SCP_START				'T'
#define SCP_STOP				'S'

#define SCP_MEMTYPE				'H'
#define SCP_MEMADDR				'J'
#define SCP_MEMWRT				'N'

#define SCP_RESET				'W'


// ********* SPEAKJET SCP REGISTERS **** //
// SCP Registers & MAX Values
#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark SCP REGISTERS & MAX VALUES
#endif

// == SCP_MEMTYPE (H) ==
#define SCP_MEMTYPE_REGISTER	'0'
#define SCP_MEMTYPE_EEPROM_H	'3'
#define SCP_MEMTYPE_EEPROM_L	'2'

// == SCP_MEMADDR (J) ==
// Envelope
#define SCP_ENV_FREQ			'0'
#define SCP_ENV_CTRL			'8'
// Oscillator Frequency Register (not used, calculated)
#define SCP_OSC1_FREQ			'1'
#define SCP_OSC2_FREQ			'2'
#define SCP_OSC3_FREQ			'3'
#define SCP_OSC4_FREQ			'4'
#define SCP_OSC5_FREQ			'5'
// Oscillator Level Register (not used, calculated)
#define SCP_OSC1_LEVEL			11
#define SCP_OSC2_LEVEL			12
#define SCP_OSC3_LEVEL			13
#define SCP_OSC4_LEVEL			14
#define SCP_OSC5_LEVEL			15
// Distortion
#define SCP_DISTORTION			'6'
// Master
#define SCP_MASTER_VOLUME		'7'

// == SCP_MEMWRT (N) ==
//  Default values
#define SCP_FREQ_DEFAULT		440
#define SCP_LEVEL_DEFAULT		20	// 63 MAX for Mixer 1: OSC 1, 2 & 3
//  Maximum accepted values
#define SCP_FREQ_MAX			3999
#define SCP_LEVEL_MAX			31
#define SCP_DISTORTION_MAX		255
#define SCP_MASTER_VOLUME_MAX	255









// ********* SPEAKJET MSA ************* //
// Mathmatical Sound Architecture
#ifdef _DEBUG_C
	#pragma mark -
	#pragma mark MSA Control Codes
#endif


// 0 - 31 CONTROL CODES
#define MSA_PAUSE0			0		// 0ms
#define MSA_PAUSE1			1		// 100ms
#define MSA_PAUSE2			2		// 200ms
#define MSA_PAUSE3			3		// 700ms
#define MSA_PAUSE4			4		// 30ms
#define MSA_PAUSE5			5		// 60ms
#define MSA_PAUSE6			6		// 90ms

#define MSA_NEXTFAST		7
#define MSA_NEXTSLOW		8
#define MSA_NEXTHIGH		14
#define MSA_NEXTLOW			15

#define MSA_WAIT			16

#define MSA_VOLUME			20
#define MSA_SPEED			21
#define MSA_PITCH			22
#define MSA_BEND			23

#define MSA_PORTCTR			24
#define MSA_PORT			25

#define MSA_REPEAT			26

#define MSA_CALLPHRASE		28
#define MSA_GOTOPHRASE		29

#define MSA_DELAY			30
#define MSA_RESET			31

// 32 - 127 (Reserved)
#ifdef _DEBUG_C
	#pragma mark MSA (Reserved)
#endif

// 128 - 254 SOUNDCODES
#ifdef _DEBUG_C
	#pragma mark MSA Sound Codes
#endif

// MSA Sound Codes: Phonemes
#define MSAPH_IY			128
#define MSAPH_IH			129
#define MSAPH_EY			130
#define MSAPH_EH			131
#define MSAPH_AY			132
#define MSAPH_AX			133
#define MSAPH_UX			134
#define MSAPH_OH			135
#define MSAPH_AW			136
#define MSAPH_OW			137
#define MSAPH_UH			138
#define MSAPH_UW			139
#define MSAPH_MM			140
#define MSAPH_NE			141
#define MSAPH_NO			142
#define MSAPH_NGE			143
#define MSAPH_NGO			144
#define MSAPH_LE			145
#define MSAPH_LO			146
#define MSAPH_WW			147
#define MSAPH_RR			148
#define MSAPH_IYRR			149
#define MSAPH_EYRR			150
#define MSAPH_AXRR			151
#define MSAPH_AWRR			152
#define MSAPH_OWRR			153
#define MSAPH_EYIY			154
#define MSAPH_OHIY			155
#define MSAPH_OWIY			156
#define MSAPH_OHIH			157
#define MSAPH_IYEH			158
#define MSAPH_EHLL			159
#define MSAPH_IYUW			160
#define MSAPH_AXUW			161
#define MSAPH_IHWW			162
#define MSAPH_AYWW			163
#define MSAPH_OWWW			164
#define MSAPH_JH			165
#define MSAPH_VV			166
#define MSAPH_ZZ			167
#define MSAPH_ZH			168
#define MSAPH_DH			169
#define MSAPH_BE			170
#define MSAPH_BO			171
#define MSAPH_EB			172
#define MSAPH_OB			173
#define MSAPH_DE			174
#define MSAPH_DO			175
#define MSAPH_ED			176
#define MSAPH_OD			177
#define MSAPH_GE			178
#define MSAPH_GO			179
#define MSAPH_EG			180
#define MSAPH_OG			181
#define MSAPH_CH			182
#define MSAPH_HE			183
#define MSAPH_HO			184
#define MSAPH_WH			185
#define MSAPH_FF			186
#define MSAPH_SE			187
#define MSAPH_SO			188
#define MSAPH_SH			189
#define MSAPH_TH			190
#define MSAPH_TT			191
#define MSAPH_TU			192
#define MSAPH_TS			193
#define MSAPH_KE			194
#define MSAPH_KO			195
#define MSAPH_EK			196
#define MSAPH_OK			197
#define MSAPH_PE			198
#define MSAPH_PO			199

// MSA Sound Codes: Robot
#define MSAFX_ROBOT_0		200
#define MSAFX_ROBOT_1		201
#define MSAFX_ROBOT_2		202
#define MSAFX_ROBOT_3		203
#define MSAFX_ROBOT_4		204
#define MSAFX_ROBOT_5		205
#define MSAFX_ROBOT_6		206
#define MSAFX_ROBOT_7		207
#define MSAFX_ROBOT_8		208
#define MSAFX_ROBOT_9		209

// MSA Sound Codes: Alarms
#define MSAFX_ALARM_0		210
#define MSAFX_ALARM_1		211
#define MSAFX_ALARM_2		212
#define MSAFX_ALARM_3		213
#define MSAFX_ALARM_4		214
#define MSAFX_ALARM_5		215
#define MSAFX_ALARM_6		216
#define MSAFX_ALARM_7		217
#define MSAFX_ALARM_8		218
#define MSAFX_ALARM_9		219

// MSA Sound Codes: Beeps
#define MSAFX_BEEP_0		220
#define MSAFX_BEEP_1		221
#define MSAFX_BEEP_2		222
#define MSAFX_BEEP_3		223
#define MSAFX_BEEP_4		224
#define MSAFX_BEEP_5		225
#define MSAFX_BEEP_6		226
#define MSAFX_BEEP_7		227
#define MSAFX_BEEP_8		228
#define MSAFX_BEEP_9		229

// MSA Sound Codes: Biological
#define MSAFX_BIO_0			230
#define MSAFX_BIO_1			231
#define MSAFX_BIO_2			232
#define MSAFX_BIO_3			233
#define MSAFX_BIO_4			234
#define MSAFX_BIO_5			235
#define MSAFX_BIO_6			236
#define MSAFX_BIO_7			237
#define MSAFX_BIO_8			238
#define MSAFX_BIO_9			239

// MSA Sound Codes: DTMF
#define MSAFX_DTMF_0		240
#define MSAFX_DTMF_1		241
#define MSAFX_DTMF_2		242
#define MSAFX_DTMF_3		243
#define MSAFX_DTMF_4		244
#define MSAFX_DTMF_5		245
#define MSAFX_DTMF_6		246
#define MSAFX_DTMF_7		247
#define MSAFX_DTMF_8		248
#define MSAFX_DTMF_9		249
#define MSAFX_DTMF_S		250
#define MSAFX_DTMF_R		251

// MSA Sound Codes: Misc
#define MSAFX_SONAR_PING	252
#define MSAFX_PISTOLSHOT	253
#define MSAFX_WOW			254

// 255: End of Phrase
#ifdef _DEBUG_C
	#pragma mark MSA EOP
#endif

#define MSA_EOP				255










// <<--- Application related defines --->>

// SCP ControlTypes
// realtime announces single SCP messages like "Clear Buffer"
// register is used for 3-part-msgs: select register, select memtype and write value
#define SCP_CTRLTYPE_REALTIME		0x0
#define SCP_CTRLTYPE_REGISTER		0x1

// Articulation ControlTypes
#define ARTICULATION_JAW			0x1
#define ARTICULATION_TONGUE			0x2

#define PHONEME_NONE				0x00
#define PHONEME_VOWEL				0x10
#define PHONEME_DIPHTONG			0x11
#define PHONEME_CONSONANT			0x20
#define PHONEME_CONSONANT_OPEN		0x21
#define PHONEME_CONSONANT_CLOSE		0x22


// OSC Harmonic Waveshapes
#define OSCSYNTH_WAVE_SAW			0
#define OSCSYNTH_WAVE_SINE			1
#define OSCSYNTH_WAVE_TRIANGLE		2
#define OSCSYNTH_WAVE_SQUARE		3

#define OSCSYNTH_ENV_OFF			0x0
#define OSCSYNTH_ENV_BASE			0x1
#define OSCSYNTH_ENV_DETUNE			0x2


#endif /* _IIC_SPEAKJETDEFINES_H */
