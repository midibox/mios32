$Id$

MIDI Force-To-Scale
===============================================================================
Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17

Optional hardware:
   o DINX1 + 4 buttons
   o LCD

===============================================================================

This application forces incoming MIDI notes to a selectable scale.

Scale and Root note can be changed via DIN buttons, each inc/dec function is
assigned to two buttons for higher flexibility with existing control surfaces:

DIN 0 and 4: Scale Dec
DIN 1 and 5: Scale Inc
DIN 2 and 6: Root Note Dec
DIN 3 and 7: Root Note Inc

The Scale can also be changed via CC#16 (0-127)
And the root note via CC#17 (0-11)


List of Root Notes:

CC#17  Note
    0  C
    1  C#
    2  D
    3  D#
    4  E
    5  F
    6  F#
    7  G
    8  G#
    9  A
   10  A#
   11  B


List of Scales (they have been provided by StrydOne - thank you!!!)
CC#16  Scale
   0  No Scale
   1  Major               
   2  Harmonic Minor      
   3  Melodic Minor       
   4  Natural Minor       
   5  Chromatic           
   6  Whole Tone          
   7  Pentatonic Major    
   8  Pentatonic Minor    
   9  Pentatonic Blues    
  10  Pentatonic Neutral  
  11  Octatonic (H-W)     
  12  Octatonic (W-H)     
  13  Ionian              
  14  Dorian              
  15  Phrygian            
  16  Lydian              
  17  Mixolydian          
  18  Aeolian             
  19  Locrian             
  20  Algerian            
  21  Arabian (A)         
  22  Arabian (B)         
  23  Augmented           
  24  Auxiliary Diminished
  25  Auxiliary Augmented 
  26  Auxiliary Diminished
  27  Balinese            
  28  Blues               
  29  Byzantine           
  30  Chinese             
  31  Chinese Mongolian   
  32  Diatonic            
  33  Diminished          
  34  Diminished, Half    
  35  Diminished, Whole   
  36  Diminished WholeTone
  37  Dominant 7th        
  38  Double Harmonic     
  39  Egyptian            
  40  Eight Tone Spanish  
  41  Enigmatic           
  42  Ethiopian (A raray) 
  43  Ethiopian Geez Ezel 
  44  Half Dim (Locrian)  
  45  Half Dim 2 (Locrian)
  46  Hawaiian            
  47  Hindu               
  48  Hindustan           
  49  Hirajoshi           
  50  Hungarian Major     
  51  Hungarian Gypsy     
  52  Hungarian G. Persian
  53  Hungarian Minor     
  54  Japanese (A)        
  55  Japanese (B)        
  56  Japan. (Ichikosucho)
  57  Japan. (Taishikicho)
  58  Javanese            
  59  Jewish(AdonaiMalakh)
  60  Jewish (Ahaba Rabba)
  61  Jewish (Magen Abot) 
  62  Kumoi               
  63  Leading Whole Tone  
  64  Lydian Augmented    
  65  Lydian Minor        
  66  Lydian Diminished   
  67  Major Locrian       
  68  Mohammedan          
  69  Neopolitan          
  70  Neoploitan Major    
  71  Neopolitan Minor    
  72  Nine Tone Scale     
  73  Oriental (A)        
  74  Oriental (B)        
  75  Overtone            
  76  Overtone Dominant   
  77  Pelog               
  78  Persian             
  79  Prometheus          
  80  PrometheusNeopolitan
  81  Pure Minor          
  82  Purvi Theta         
  83  Roumanian Minor     
  84  Six Tone Symmetrical
  85  Spanish Gypsy       
  86  Super Locrian       
  87  Theta, Asavari      
  88  Theta, Bilaval      
  89  Theta, Bhairav      
  90  Theta, Bhairavi     
  91  Theta, Kafi         
  92  Theta, Kalyan       
  93  Theta, Khamaj       
  94  Theta, Marva        
  95  Todi Theta          
  96  M. Bhavapriya 44    
  97  M. Chakravakam 16   
  98  M. Chalanata 36     
  99  M. Charukesi 26     
 100  M. Chitrambari 66   
 101  M. Dharmavati 59    
 102  M. Dhatuvardhani 69 
 103  M. Dhavalambari 49  
 104  M. Dhenuka 9        
 105  M. Dhirasankarabhara
 106  M. Divyamani 48     
 107  M. Gamanasrama 53   
 108  M. Ganamurti 3      
 109  M. Gangeyabhusani 33
 110  M. Gaurimanohari 23 
 111  M. Gavambodhi 43    
 112  M. Gayakapriya 13   
 113  M. Hanumattodi 8    
 114  M. Harikambhoji 28  
 115  M. Hatakambari 18   
 116  M. Hemavati 58      
 117  M. Jalarnavam 38    
 118  M. Jhalavarali 39   
 119  M. Jhankaradhvani 19
 120  M. Jyotisvarupini 68
 121  M. Kamavardhani 51  
 122  M. Kanakangi 1      
 123  M. Kantamani 61     
 124  M. Kharaharapriya 22
 125  M. Kiravani 21      
 126  M. Kokilapriya 11   
 127  M. Kosalam 71       

===============================================================================
