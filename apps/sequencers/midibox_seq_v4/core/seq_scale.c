// $Id$
/*
 * Force-to-Scale Functions
 *
 * The extra large scale table has been provided by StrydOne - Thank You!
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

#include "seq_scale.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned c:4;
    unsigned cd:4;
    unsigned d:4;
    unsigned dd:4;
    unsigned e:4;
    unsigned f:4;
    unsigned fd:4;
    unsigned g:4;
    unsigned gd:4;
    unsigned a:4;
    unsigned ad:4;
    unsigned b:4;
    char name[21];
  };

  struct {
    u8 notes[6]; // unfortunately "unsigned notes:4[12]" doesn't work
  };
} seq_scale_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

const seq_scale_entry_t seq_scale_table[] = {
//	        C 	C#	D 	D#	E 	F 	F#	G 	G#	A 	A#	B
//	        0 	1 	2 	3 	4 	5 	6 	7 	8 	9 	10	11 	Semitone
//              1 	b2	2 	b3	3 	4 	b5	5 	b6	6 	b7	7	Minor Tone
//              1 	#1	2 	#2	3 	4 	#4	5 	#5	6 	#6	7 	Augmented Tone
// 1..10                                                                                                         <------------------>
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	11,	11,	"Major               " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	11,	11,	"Harmonic Minor      " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	9,	9,	11,	11,	"Melodic Minor       " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Natural Minor       " },
  {             0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	"Chromatic           " },
  {             0,	2,	2,	4,	4,	6,	6,	8,	8,	10,	10,	10,	"Whole Tone          " },
  {             0,	2,	2,	4,	4,	7,	7,	7,	9,	9,	9,	9,	"Pentatonic Major    " },
  {             0,	0,	3,	3,	3,	5,	7,	7,	7,	10,	10,	10,	"Pentatonic Minor    " },
  {             0,	0,	3,	3,	3,	5,	6,	7,	7,	10,	10,	10,	"Pentatonic Blues    " },
  {             0,	2,	2,	2,	5,	5,	7,	7,	7,	10,	10,	10,	"Pentatonic Neutral  " },
// 11..20                                                                                                        <------------------>
  {             0,	1,	1,	3,	4,	6,	6,	7,	9,	9,	10,	10,	"Octatonic (H-W)     " },
  {             0,	2,	2,	3,	3,	5,	6,	6,	8,	9,	11,	11,	"Octatonic (W-H)     " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	11,	11,	"Ionian              " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	9,	9,	10,	10,	"Dorian              " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Phrygian            " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	9,	9,	11,	11,	"Lydian              " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	10,	10,	"Mixolydian          " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Aeolian             " },
  {             0,	1,	1,	3,	3,	5,	6,	6,	8,	8,	10,	10,	"Locrian             " },
  {             0,	2,	2,	3,	3,	5,	6,	7,	8,	8,	11,	11,	"Algerian            " },
// 21..30                                                                                                        <------------------>
  {             0,	2,	2,	3,	3,	5,	6,	8,	8,	9,	11,	11,	"Arabian (A)         " },
  {             0,	2,	2,	4,	4,	5,	6,	8,	8,	10,	10,	10,	"Arabian (B)         " },
  {             0,	3,	3,	3,	4,	6,	6,	8,	8,	11,	11,	11,	"Augmented           " },
  {             0,	2,	2,	3,	3,	5,	6,	8,	8,	9,	11,	11,	"Auxiliary Diminished" },
  {             0,	2,	2,	4,	4,	6,	6,	8,	8,	10,	10,	10,	"Auxiliary Augmented " },
  {             0,	1,	1,	3,	4,	6,	6,	7,	9,	9,	10,	10,	"Auxiliary Diminished" },
  {             0,	1,	1,	3,	3,	7,	7,	7,	8,	8,	8,	8,	"Balinese            " },
  {             0,	0,	3,	3,	3,	5,	6,	7,	7,	10,	10,	10,	"Blues               " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	8,	11,	11,	"Byzantine           " },
  {             0,	0,	4,	4,	4,	6,	6,	7,	7,	11,	11,	11,	"Chinese             " },
// 31..40                                                                                                        <------------------>
  {             0,	2,	2,	4,	4,	7,	7,	7,	9,	9,	9,	9,	"Chinese Mongolian   " },
  {             0,	2,	2,	4,	4,	7,	7,	7,	9,	9,	9,	9,	"Diatonic            " },
  {             0,	2,	2,	3,	3,	5,	6,	6,	8,	9,	11,	11,	"Diminished          " },
  {             0,	1,	1,	3,	4,	6,	6,	7,	9,	9,	10,	10,	"Diminished, Half    " },
  {             0,	2,	2,	3,	3,	5,	6,	6,	8,	9,	11,	11,	"Diminished, Whole   " },
  {             0,	1,	1,	3,	4,	6,	6,	6,	8,	8,	10,	10,	"Diminished WholeTone" },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	10,	10,	"Dominant 7th        " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	8,	11,	11,	"Double Harmonic     " },
  {             0,	2,	2,	2,	5,	5,	7,	7,	7,	10,	10,	10,	"Egyptian            " },
  {             0,	1,	1,	3,	4,	5,	6,	6,	8,	8,	10,	10,	"Eight Tone Spanish  " },
// 41..50                                                                                                        <------------------>
  {             0,	1,	1,	4,	4,	6,	6,	8,	8,	10,	10,	11,	"Enigmatic           " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	11,	11,	"Ethiopian (A raray) " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Ethiopian Geez Ezel " },
  {             0,	1,	1,	3,	3,	5,	6,	6,	8,	8,	10,	10,	"Half Dim (Locrian)  " },
  {             0,	2,	2,	3,	3,	5,	6,	6,	8,	8,	10,	10,	"Half Dim 2 (Locrian)" },
  {             0,	2,	2,	3,	3,	5,	7,	7,	9,	9,	11,	11,	"Hawaiian            " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	8,	8,	10,	10,	"Hindu               " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	8,	8,	10,	10,	"Hindustan           " },
  {             0,	2,	2,	3,	3,	7,	7,	7,	8,	8,	8,	8,	"Hirajoshi           " },
  {             0,	3,	3,	3,	4,	6,	6,	7,	9,	9,	10,	10,	"Hungarian Major     " },
// 51..60                                                                                                        <------------------>
  {             0,	2,	2,	3,	3,	6,	6,	7,	8,	8,	11,	11,	"Hungarian Gypsy     " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	8,	11,	11,	"Hungarian G. Persian" },
  {             0,	2,	2,	3,	3,	6,	6,	7,	8,	8,	11,	11,	"Hungarian Minor     " },
  {             0,	1,	1,	1,	5,	5,	7,	7,	8,	8,	8,	8,	"Japanese (A)        " },
  {             0,	2,	2,	2,	5,	5,	7,	7,	8,	8,	8,	8,	"Japanese (B)        " },
  {             0,	2,	2,	4,	4,	5,	6,	7,	9,	9,	11,	11,	"Japan. (Ichikosucho)" },
  {             0,	2,	2,	4,	4,	5,	6,	7,	9,	9,	10,	11,	"Japan. (Taishikicho)" },
  {             0,	1,	1,	3,	3,	5,	7,	7,	9,	9,	10,	10,	"Javanese            " },
  {             0,	1,	2,	3,	3,	5,	7,	7,	9,	9,	10,	10,	"Jewish(AdonaiMalakh)" },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	8,	10,	10,	"Jewish (Ahaba Rabba)" },
// 61..70                                                                                                        <------------------>
  {             0,	1,	1,	3,	4,	6,	6,	8,	8,	10,	10,	11,	"Jewish (Magen Abot) " },
  {             0,	2,	2,	3,	3,	7,	7,	7,	9,	9,	9,	9,	"Kumoi               " },
  {             0,	2,	2,	4,	4,	6,	6,	8,	8,	10,	10,	11,	"Leading Whole Tone  " },
  {             0,	2,	2,	4,	4,	6,	6,	8,	8,	9,	11,	11,	"Lydian Augmented    " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	8,	8,	10,	10,	"Lydian Minor        " },
  {             0,	2,	2,	3,	3,	6,	6,	7,	9,	9,	11,	11,	"Lydian Diminished   " },
  {             0,	2,	2,	4,	4,	5,	6,	6,	8,	8,	10,	10,	"Major Locrian       " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	11,	11,	"Mohammedan          " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	8,	8,	11,	11,	"Neopolitan          " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	9,	9,	11,	11,	"Neoploitan Major    " },
// 71..80                                                                                                        <------------------>
  {             0,	1,	1,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Neopolitan Minor    " },
  {             0,	2,	2,	3,	4,	6,	6,	7,	8,	9,	11,	11,	"Nine Tone Scale     " },
  {             0,	1,	1,	4,	4,	5,	6,	6,	8,	8,	10,	10,	"Oriental (A)        " },
  {             0,	1,	1,	4,	4,	5,	6,	6,	9,	9,	10,	10,	"Oriental (B)        " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	9,	9,	10,	10,	"Overtone            " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	9,	9,	10,	10,	"Overtone Dominant   " },
  {             0,	1,	1,	3,	3,	7,	7,	7,	8,	8,	8,	8,	"Pelog               " },
  {             0,	1,	1,	4,	4,	5,	6,	6,	8,	8,	11,	11,	"Persian             " },
  {             0,	2,	2,	4,	4,	6,	6,	6,	9,	9,	10,	10,	"Prometheus          " },
  {             0,	1,	1,	4,	4,	6,	6,	6,	9,	9,	10,	10,	"PrometheusNeopolitan" },
// 81..90                                                                                                        <------------------>
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Pure Minor          " },
  {             0,	1,	1,	4,	4,	6,	6,	7,	8,	8,	11,	11,	"Purvi Theta         " },
  {             0,	2,	2,	3,	3,	6,	6,	7,	9,	9,	10,	10,	"Roumanian Minor     " },
  {             0,	1,	1,	4,	4,	5,	8,	8,	8,	9,	9,	9,	"Six Tone Symmetrical" },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	8,	10,	10,	"Spanish Gypsy       " },
  {             0,	1,	1,	3,	4,	6,	6,	8,	8,	10,	10,	10,	"Super Locrian       " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Theta, Asavari      " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	11,	11,	"Theta, Bilaval      " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	8,	11,	11,	"Theta, Bhairav      " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"Theta, Bhairavi     " },
// 91..100                                                                                                        <------------------>
  {             0,	2,	2,	3,	3,	5,	7,	7,	9,	9,	10,	10,	"Theta, Kafi         " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	9,	9,	11,	11,	"Theta, Kalyan       " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	10,	10,	"Theta, Khamaj       " },
  {             0,	1,	1,	4,	4,	6,	6,	7,	9,	9,	11,	11,	"Theta, Marva        " },
  {             0,	1,	1,	3,	3,	6,	6,	7,	8,	8,	11,	11,	"Todi Theta          " },
  {             0,	1,	2,	2,	5,	5,	7,	7,	8,	9,	9,	9,	"M. Bhavapriya 44    " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	9,	9,	10,	10,	"M. Chakravakam 16   " },
  {             0,	3,	3,	3,	4,	5,	7,	7,	10,	10,	10,	11,	"M. Chalanata 36     " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	8,	8,	10,	10,	"M. Charukesi 26     " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	10,	10,	10,	11,	"M. Chitrambari 66   " },
// 101..110                                                                                                        <------------------>
  {             0,	2,	2,	3,	3,	6,	6,	7,	9,	9,	11,	11,	"M. Dharmavati 59    " },
  {             0,	3,	3,	3,	4,	6,	6,	7,	8,	8,	11,	11,	"M. Dhatuvardhani 69 " },
  {             0,	1,	1,	4,	4,	6,	6,	7,	8,	9,	9,	9,	"M. Dhavalambari 49  " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	8,	8,	11,	11,	"M. Dhenuka 9        " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	11,	11,	"M. Dhirasankarabhara" },
  {             0,	1,	1,	3,	3,	6,	6,	7,	10,	10,	10,	11,	"M. Divyamani 48     " },
  {             0,	1,	1,	4,	4,	6,	6,	7,	9,	9,	11,	11,	"M. Gamanasrama 53   " },
  {             0,	1,	2,	2,	5,	5,	7,	7,	8,	8,	11,	11,	"M. Ganamurti 3      " },
  {             0,	3,	3,	3,	4,	5,	7,	7,	8,	8,	11,	11,	"M. Gangeyabhusani 33" },
  {             0,	2,	2,	3,	3,	5,	7,	7,	9,	9,	11,	11,	"M. Gaurimanohari 23 " },
// 111..120                                                                                                        <------------------>
  {             0,	1,	1,	3,	3,	6,	6,	7,	8,	9,	9,	9,	"M. Gavambodhi 43    " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	9,	9,	9,	"M. Gayakapriya 13   " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"M. Hanumattodi 8    " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	9,	9,	10,	10,	"M. Harikambhoji 28  " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	10,	10,	10,	11,	"M. Hatakambari 18   " },
  {             0,	2,	2,	3,	3,	6,	6,	7,	9,	9,	10,	10,	"M. Hemavati 58      " },
  {             0,	1,	2,	2,	6,	6,	6,	7,	8,	8,	10,	10,	"M. Jalarnavam 38    " },
  {             0,	1,	2,	2,	6,	6,	6,	7,	9,	9,	10,	10,	"M. Jhalavarali 39   " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	9,	9,	10,	10,	"M. Jhankaradhvani 19" },
  {             0,	3,	3,	3,	4,	6,	6,	7,	8,	8,	10,	10,	"M. Jyotisvarupini 68" },
// 121..127                                                                                                        <------------------>
  {             0,	1,	1,	4,	4,	6,	6,	7,	8,	8,	11,	11,	"M. Kamavardhani 51  " },
  {             0,	1,	2,	2,	5,	5,	7,	7,	8,	9,	9,	9,	"M. Kanakangi 1      " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	8,	9,	9,	9,	"M. Kantamani 61     " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	9,	9,	10,	10,	"M. Kharaharapriya 22" },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	11,	11,	"M. Kiravani 21      " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	9,	9,	11,	11,	"M. Kokilapriya 11   " },
  {             0,	3,	3,	3,	4,	6,	6,	7,	9,	9,	11,	11,	"M. Kosalam 71       " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	8,	8,	11,	11,	"M. Latangi 63       " },

  {             0,	1,	2,	2,	5,	5,	7,	7,	9,	9,	11,	11,	"M. Manavati 5       " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	8,	9,	9,	9,	"M. Mararanjani 25   " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	8,	9,	9,	9,	"M. Mayamalavagaula 1" },
  {             0,	2,	2,	4,	4,	6,	6,	7,	9,	9,	11,	11,	"M. Mechakalyani 65  " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	10,	10,	10,	11,	"M. Naganandini 30   " },
  {             0,	1,	1,	4,	4,	6,	6,	7,	8,	8,	10,	10,	"M. Namanarayani 50  " },
  {             0,	3,	3,	3,	4,	6,	6,	7,	9,	9,	10,	10,	"M. Nasikabhusani 70 " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	8,	8,	10,	10,	"M. Natabhairavi 20  " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	9,	9,	10,	10,	"M. Natakapriya 10   " },
  {             0,	1,	2,	2,	6,	6,	6,	7,	9,	9,	10,	10,	"M. Navanitam 40     " },
  {             0,	2,	2,	3,	3,	6,	6,	7,	10,	10,	10,	11,	"M. Nitimati 60      " },
  {             0,	1,	2,	2,	6,	6,	6,	7,	9,	9,	11,	11,	"M. Pavani 41        " },
  {             0,	3,	3,	3,	4,	5,	7,	7,	8,	8,	10,	10,	"M. Ragavardhani 32  " },
  {             0,	1,	2,	2,	6,	6,	6,	7,	10,	10,	10,	11,	"M. Raghupriya 42    " },
  {             0,	1,	1,	4,	4,	6,	6,	7,	9,	9,	10,	10,	"M. Ramapriya 52     " },
  {             0,	3,	3,	3,	4,	6,	6,	7,	10,	10,	10,	11,	"M. Rasikapriya 72   " },
  {             0,	1,	2,	2,	5,	5,	7,	7,	8,	8,	10,	10,	"M. Ratnangi 2       " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	8,	8,	10,	10,	"M. Risabhapriya 62  " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	10,	10,	10,	11,	"M. Rupavati 12      " },
  {             0,	1,	1,	3,	3,	6,	6,	7,	9,	9,	10,	10,	"M. Sadvidhamargini 4" },
  {             0,	1,	2,	2,	6,	6,	6,	7,	8,	9,	9,	9,	"M. Salagam 37       " },
  {             0,	2,	2,	3,	3,	6,	6,	7,	8,	8,	10,	10,	"M. Sanmukhapriya 56 " },
  {             0,	2,	2,	4,	4,	5,	7,	7,	8,	8,	11,	11,	"M. Sarasangi 27     " },
  {             0,	1,	1,	3,	3,	5,	7,	7,	8,	9,	9,	9,	"M. Senavati 7       " },
  {             0,	2,	2,	3,	3,	6,	6,	7,	8,	8,	11,	11,	"M. Simhendramadhyama" },
  {             0,	1,	1,	3,	3,	6,	6,	7,	8,	8,	11,	11,	"M. Subhapantuvarali " },
  {             0,	3,	3,	3,	4,	6,	6,	7,	8,	9,	9,	9,	"M. Sucharitra 67    " },
  {             0,	3,	3,	3,	4,	5,	7,	7,	9,	9,	11,	11,	"M. Sulini 35        " },
  {             0,	1,	1,	4,	4,	5,	7,	7,	9,	9,	11,	11,	"M. Suryakantam 17   " },
  {             0,	1,	2,	2,	6,	6,	6,	7,	9,	9,	11,	11,	"M. Suvarnangi 47    " },
  {             0,	2,	2,	3,	3,	6,	6,	7,	8,	9,	9,	9,	"M. Syamalangi 55    " },
  {             0,	1,	2,	2,	5,	5,	7,	7,	10,	10,	10,	11,	"M. Tanarupi 6       " },
  {             0,	2,	2,	4,	4,	6,	6,	7,	9,	9,	10,	10,	"M. Vaschaspati 64   " },
  {             0,	3,	3,	3,	4,	5,	7,	7,	9,	9,	10,	10,	"M. Vagadhisvari 34  " },
  {             0,	3,	3,	3,	4,	5,	7,	7,	8,	8,	10,	10,	"M. Vakulabharanam 14" },
  {             0,	1,	2,	2,	5,	5,	7,	7,	9,	9,	10,	10,	"M. Vanaspati 4      " },
  {             0,	2,	2,	3,	3,	5,	7,	7,	10,	10,	10,	11,	"M. Varunapriya 24   " },
  {             0,	3,	3,	3,	4,	6,	6,	7,	10,	10,	10,	11,	"M. Visvambari 54    " },
  {             0,	3,	3,	3,	4,	5,	7,	7,	8,	9,	9,	9,	"M. Yagapriya 31     " }
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SCALE_Init(u32 mode)
{
  // here we could also generate the scale table in RAM...

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns number of available scales
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SCALE_GetNum(void)
{
  return sizeof(seq_scale_table)/sizeof(seq_scale_entry_t);
}


/////////////////////////////////////////////////////////////////////////////
// returns pointer to the name of a scale
// Length: 20 characters + zero terminator
/////////////////////////////////////////////////////////////////////////////
char *SEQ_SCALE_GetName(u8 scale)
{
  if( scale >= SEQ_SCALE_GetNum() )
    return "Invalid Scale       ";

  return seq_scale_table[scale].name;
}


/////////////////////////////////////////////////////////////////////////////
// This function is called if a note value should be forced to the globally
// selected scale
// IN: *p: midi note (a mios32_midi_package_t) - will be modified, 
//     therefore passed as pointer
//     scale: within the range 0..SEQ_SCALE_GetNum()-1
//     root: the root note (0..11)
// returns modified note in *p
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SCALE_Note(mios32_midi_package_t *p, u8 scale, u8 root)
{
  // exit if no Note event
  if( p->type != NoteOn && p->type != NoteOff )
    return -1;

  // normalize note and determine octave
  int note_number = p->note - root;
  if( note_number < 0 ) note_number += 12;
  u8 octave = note_number / 12;
  note_number = note_number % 12;

  // get scaled value from table
  u8 tmp = seq_scale_table[scale].notes[note_number>>1];
  u8 note_scaled = (note_number & 1) ? (tmp >> 4) : (tmp & 0xf);

  // add octave and root note
  note_scaled += 12*octave + root;

  // decrement octave so long note number outside allowed range
  while( note_scaled >= 128 )
    note_scaled -= 12;

  // replace note number in package
  p->note = note_scaled;

  return 0; // no error
}
