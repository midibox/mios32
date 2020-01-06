/*
 * Pattern Remix page
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Romulo Silva (contato@romulosilva.org) 
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include <seq_bpm.h>
#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_mixer.h"

#include "tasks.h"
#include "file.h"
#include "seq_file_t.h"
#include "seq_file.h"
#include "seq_file_b.h"
#include "seq_core.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PAGE_MAIN        0
#define PAGE_REMIX       1
#define PAGE_OPTION      2
#define PAGE_NAME_EDIT   3
#define PAGE_TRK_DELAY   4

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_page = PAGE_MAIN;

static seq_pattern_t selected_pattern[SEQ_CORE_NUM_GROUPS];

// default ableton api port insi
// TODO: put this inside options page to be selected by the user
static mios32_midi_port_t ableton_port = USB0;

// preview function
static u8 preview_mode = 0;
static u8 remix_mode = 0;
static u8 preview_bank;
static u8 preview_num;
static char preview_name[21];
static char pattern_name[21];
static char pattern_name_copypaste[21];
static u16 seq_pattern_demix_map = 0;

// option parameters
static u8 auto_save = 0;
//static u8 auto_reset = 0;
static u8 ableton_api = 1;

// static variables (only used here, therefore local)
static u32 pc_time_control = 0; // timestamp of last operation

//static mios32_sys_time_t pattern_timer;
static mios32_sys_time_t pattern_remix_timer;

/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{

  if( SEQ_FILE_FormattingRequired() )
    return 0; // no LED action as long as files not available

  switch( selected_page ) {

    ///////////////////////////////////////////////////////////////////////////
    // REMIX PAGE
    case PAGE_REMIX: {

			*gp_leds = seq_pattern_remix_map;
			
    } break;

    ///////////////////////////////////////////////////////////////////////////
    // OPTION PAGE
    case PAGE_OPTION: {

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // PATTERN NAME EDIT PAGE
    case PAGE_NAME_EDIT: {

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // TRACK DELAY PAGE
    case PAGE_TRK_DELAY: {

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // MAIN PAGE
    case PAGE_MAIN: {

      seq_pattern_t pattern = (ui_selected_item == (ui_selected_group) && ui_cursor_flash) 

        ? seq_pattern[ui_selected_group] : selected_pattern[ui_selected_group];
			
      *gp_leds = (1 << pattern.group) | (1 << (pattern.num+8));

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // Page do not exist
    default: {
		
		} break;

  }
	
	return 0; // no error
	
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  if( SEQ_FILE_FormattingRequired() )
    return 0; // no encoder action as long as files not available

  switch( selected_page ) {

    ///////////////////////////////////////////////////////////////////////////
    // REMIX PAGE
    case PAGE_REMIX: {
			
      // TODO: finish the incrementer helper for gp encoders
      if( encoder <= SEQ_UI_ENCODER_GP16 ) {
		
				if ( preview_mode ) { // the remix state
						
					if( ((1 << encoder) | seq_pattern_remix_map) == seq_pattern_remix_map ) { // if we already got the requested bit setup
						// unset him inside remix map
						seq_pattern_remix_map &= ~(1 << encoder);
					} else { // else setup it!
						// set him inside remix map
						seq_pattern_remix_map |= (1 << encoder);
					}
						
				} else { // the demix state
					
					// in this state we can not mix, just turn mixed states track into demixed state
					if( ((1 << encoder) | seq_pattern_remix_map) == seq_pattern_remix_map ) { // if we already got the requested bit setup inside remix map
						// unset him inside remix map
						seq_pattern_remix_map &= ~(1 << encoder);
					} else if ( ((1 << encoder) | seq_pattern_demix_map) == seq_pattern_demix_map ) { // if we already got the requested bit setup demix map
						// set him inside remix map
						seq_pattern_remix_map |= (1 << encoder);
					}
						
			  }
				
				if ( !preview_mode && ableton_api) {
					// send slot play envet to ableton cc (111 + track) with value 127 to channel 16
					MIOS32_MIDI_SendCC(ableton_port, 15, (111 + encoder), 127);	
				}
				
      }			
			
    } break;

    ///////////////////////////////////////////////////////////////////////////
    // OPTION PAGE
    case PAGE_OPTION: {

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // PATTERN NAME EDIT PAGE
    case PAGE_NAME_EDIT: {

      u8 group;
      s32 status;

      // if we got a OK button pressed
      if (encoder==SEQ_UI_ENCODER_GP16) {
        // Get back to main page
        selected_page = PAGE_MAIN;
        return 1;
      }

      if( (status=SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&seq_pattern_name[0], 20)) < 0 ) {
        // error
        return 0;
      } else {
        // copy the name of group 1 pattern to all other pattern groups
        for(group=1; group<SEQ_CORE_NUM_GROUPS; ++group) {
					//SEQ_LABEL_CopyPresetCategory(seq_pattern[0].num, (char *)&seq_pattern_name[group][0]);
					sprintf(seq_pattern_name[group], seq_pattern_name[0]);
        }
				
				// copy the pattern name
				sprintf(pattern_name, seq_pattern_name[0]);
				
      }

      return 1;

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // TRACK DELAY PAGE
    case PAGE_TRK_DELAY: {

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // MAIN PAGE
    case PAGE_MAIN: {

      switch( encoder ) {

        case SEQ_UI_ENCODER_Datawheel:
          incrementer = incrementer;

          u16 value = (u16)(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num]*10);
          if( SEQ_UI_Var16_Inc(&value, 25, 3000, incrementer) ) { // at 384ppqn, the minimum BPM rate is ca. 2.5
            // set new BPM
            seq_core_bpm_preset_tempo[seq_core_bpm_preset_num] = (float)value/10.0;
            SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);
            return 1; // value has been changed
          } else {
            return 0; // value hasn't been changed
          }

        default:
          return -1; // invalid or unsupported encoder
      }

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // Page do not exist
    default:
      break;

  }
	
	return 0;

}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{

  if( SEQ_FILE_FormattingRequired() )
    return 0; // no button action as long as files not available
	
	u8 group;
	s32 status;
	
  switch( button ) {
		
		case SEQ_UI_BUTTON_Edit: {
			
			
			if ( depressed && (selected_page == PAGE_REMIX) ) {
					
				// we want to show vertical VU meters normal mode
				SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);
				
				selected_page = PAGE_MAIN;		
				
				//
				// here we run the commands for remix states
				//
				u16 remix_map_tmp;
				
				// if we are in no preview_mode them call the demix procedment in case we got some request
				if ( (seq_pattern_remix_map != seq_pattern_demix_map) && ( remix_mode ) ) {
					remix_map_tmp = seq_pattern_remix_map;
					seq_pattern_remix_map = ~(seq_pattern_remix_map ^ seq_pattern_demix_map);
					
          if (ableton_api) {
						
            // TODO: implements a ableton remotescript to the clip control functionality
						
						u8 track;
						for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
						 
						 // if we got the track bit setup inside our remix_map, them do not send mixer data for that track channel, let it be mixed down
						 if ( ((1 << track) | seq_pattern_remix_map) == seq_pattern_remix_map ) {
							 // do nothing...
						 } else {
#ifndef MIOS32_FAMILY_EMULATION
							 // delay our task for ableton get a breath before change the pattern line
							 vTaskDelay(50);
#endif
							 // send slot play envet to ableton: cc (111 + track) with value 127 to channel 16
							 MIOS32_MIDI_SendCC(ableton_port, 15, (111 + track), 127);
						 }
						 
						}           
						
          }
					
					// if we are in remix mode lets start the process
					if (remix_map_tmp == 0) {
												
						//pattern_name = pattern_name_remix;
						preview_mode = 0;
						remix_mode = 0;
						// set the pattern_remix_timer reference
						pattern_remix_timer = MIOS32_SYS_TimeGet();
						
					}
					
					// Change Pattern
					u8 group;
					for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
						// change pattern
						SEQ_PATTERN_Change(group, seq_pattern[group], 0);
					}
					
					// copy the pattern name for future reference
					sprintf(pattern_name, seq_pattern_name[0]);
					
					// getting back
					seq_pattern_remix_map = remix_map_tmp;
					seq_pattern_demix_map = seq_pattern_remix_map;
					
				}
				
			} else if ( !depressed ) {		
				
				// we want to show horizontal VU meters in remix mode
				SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);
				
				selected_page = PAGE_REMIX;
			}
				
			return 1;	
				
		} break;				
			
    case SEQ_UI_BUTTON_Select: {
			
      if(!depressed) {
        selected_page = PAGE_OPTION;
        return 1;
      }
			
      if( (depressed) && (selected_page == PAGE_OPTION) ) {
        selected_page = PAGE_MAIN;
      }
			
		} break;
			
		default:
			break;
			
			
  }

  if( depressed ) return 0; // ignore when button depressed if its not SEQ_UI_BUTTON_Select or SEQ_UI_BUTTON_Edit
	
  switch( selected_page ) {

    ///////////////////////////////////////////////////////////////////////////
    // REMIX PAGE
    case PAGE_REMIX: {

      if( button <= SEQ_UI_BUTTON_GP16 ) {
        // re-using encoder routine
        return Encoder_Handler(button, 0);
      }			
			
    } break;

    ///////////////////////////////////////////////////////////////////////////
    // OPTION PAGE
    case PAGE_OPTION: {

      switch( button ) {

        case SEQ_UI_BUTTON_GP1:
          // Save Pattern(all 4 groups at once)
          for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
						
            if( (status=SEQ_PATTERN_Save(group, seq_pattern[group])) < 0 ) {
              SEQ_UI_SDCardErrMsg(2000, status);
              return 0;
            }
						
          }
					
          // Save the pattern mixer
          SEQ_MIXER_Save(SEQ_MIXER_NumGet());
					
          break;

        case SEQ_UI_BUTTON_GP2:
          // Edit Pattern Name
					//SEQ_LCD_Clear();
          selected_page = PAGE_NAME_EDIT;
          break;

        case SEQ_UI_BUTTON_GP3:
          // Copy Pattern

	  // copy the pattern name
	  if (remix_mode == 1) {
	    sprintf(pattern_name_copypaste, pattern_name);
	  } else {
	    sprintf(pattern_name_copypaste, seq_pattern_name[0]);
	  }

	  
	  // We are going to use the multicopy procedure
	  SEQ_UI_PATTERN_MultiCopy(0);

	  // coping mixer
	  SEQ_UI_MIXER_Copy();

          break;

        case SEQ_UI_BUTTON_GP4:
          // Paste Pattern

	  // We are going to use the multicopy procedure
	  SEQ_UI_PATTERN_MultiPaste(0);

	  // paste the pattern name
	  sprintf(seq_pattern_name[0], pattern_name_copypaste);

	  // paste mixer
	  SEQ_UI_MIXER_Paste();
			
          break;

        case SEQ_UI_BUTTON_GP5:
        case SEQ_UI_BUTTON_GP6:
          return -1; // not mapped

        case SEQ_UI_BUTTON_GP7:
        case SEQ_UI_BUTTON_GP8:
          // Track Delay Page
					//SEQ_LCD_Clear();
          //selected_page = PAGE_TRK_DELAY;
          //break;
					return -1; // not mapped

        case SEQ_UI_BUTTON_GP9:
        case SEQ_UI_BUTTON_GP10:
					return -1; // not mapped

        case SEQ_UI_BUTTON_GP11:
        case SEQ_UI_BUTTON_GP12:
					return -1; // not mapped

        case SEQ_UI_BUTTON_GP13:
        case SEQ_UI_BUTTON_GP14:
          // Auto Save switch
          auto_save ^= 1;
          break;
          break;

        case SEQ_UI_BUTTON_GP15:
        case SEQ_UI_BUTTON_GP16:
					// Ableton API switch
          ableton_api ^= 1;
          return -1; // not mapped
      }

      return 0;

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // PATTERN NAME EDIT PAGE
    case PAGE_NAME_EDIT: {

      return Encoder_Handler((seq_ui_encoder_t)button, 0);

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // TRACK DELAY PAGE
    case PAGE_TRK_DELAY: {

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // MAIN PAGE
    case PAGE_MAIN: {

      // Pattern Change Group Requests
      if( button <= SEQ_UI_BUTTON_GP8 ) {

        u8 group;
				
        for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
          if( selected_pattern[group].group == button ) {
            if( SEQ_FILE_B_NumPatterns(selected_pattern[group].bank) > 64 ) {
              selected_pattern[group].lower ^= 1;
            } else {
              selected_pattern[group].lower = 0; // toggling not required
            }
          } else {
            selected_pattern[group].group = button;
            preview_bank = button;
            //preview_mode = 0;
          }
        }
				
				SEQ_PATTERN_PeekName(selected_pattern[0], preview_name);
				preview_mode = 1;
				
        return 1; // value always changed
      }

      // Pattern Change Request
      if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 ) {

        //u8 group;

        s32 status = 0;

        // setting save patterns for later autosave procedure
        //for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
        //  save_pattern[group] = seq_pattern[group];
				
        // change preview pattern
        selected_pattern[0].num = button-8;
        selected_pattern[1].num = button-8;
        selected_pattern[2].num = button-8;
        selected_pattern[3].num = button-8;

        if ( !preview_mode )
        {
          preview_num = button-8;
          SEQ_PATTERN_PeekName(selected_pattern[0], preview_name);
          preview_mode = 1;
					
					// for security reasons in live environment, we can not accept rapid 2x press of pattern button.
					// if the buttons are not good quality they can send a 2x rapid signal os pressing in just one physical pressing
					// the time we need is bassicly the human time to read the name of patterns requested
			    // start time in count
	                                pc_time_control = MIOS32_TIMESTAMP_Get() + 200;
					// vTaksDelay(200);
        } else {

					// 
					if ( remix_mode == 0 ) {
					
						//char str[30];
						//sprintf(str, "%d = %d %d = %d", selected_pattern[0].num, preview_num, selected_pattern[0].group, preview_bank);
						//SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, str, "debug");
						
						// Check for remix state
						if ( (selected_pattern[0].num == preview_num) &&  (selected_pattern[0].group == preview_bank) ) {
						
							//if ( pc_time_control >= MIOS32_TIMESTAMP_Get() )
							//	return 0; // do nothing, just to be shure we are not handle acidental double clicks, it happens!					
							
							// if we got some remix bit setup, enter in remix mode
							if (seq_pattern_remix_map != 0) {
								// set the remix mode
								remix_mode = 1;
							}
						
						}
						
					}
					
          // Change Pattern
          if ( (selected_pattern[0].num == preview_num) &&  (selected_pattern[0].group == preview_bank) ) {

  	        if ( pc_time_control >= MIOS32_TIMESTAMP_Get() )
	            return 0; // do nothing, just to be shure we are not handle acidental double clicks

						if (ableton_api) {
							
							u8 ableton_pattern_number = 0;
							
							//
							// ABLETON API PATTERN CHANGE HANDLE
							//
							// send ableton event to change the line clips
							if (selected_pattern[0].lower) {
						  	ableton_pattern_number = ((selected_pattern[0].group - 1) * 8) + button;
							} else {
						  	ableton_pattern_number = (((selected_pattern[0].group - 1) + 8) * 8) + button;
							}
							
							// midi_cc to send = 110
							// midi_chn = 16
							// midi_port = ableton_port
							// here we need to send a clip line change request
							MIOS32_MIDI_SendCC(ableton_port, 15, 110, ableton_pattern_number);
						
#ifndef MIOS32_FAMILY_EMULATION
							// delay our task for ableton get a breath before change the pattern line
							vTaskDelay(50);
#endif

							// clip triggering
							u8 track;
            	for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
              
              	// if we got the track bit setup inside our remix_map, them do not send mixer data for that track channel, let it be mixed down
              	if ( ((1 << track) | seq_pattern_remix_map) == seq_pattern_remix_map ) {
                	// do nothing...
              	} else {
#ifndef MIOS32_FAMILY_EMULATION
									// delay our task for ableton get a breath before change the pattern line
									vTaskDelay(50);									
#endif
									// send clip slot play envet to ableton cc (111 + track) with value 127 to channel 16
                	MIOS32_MIDI_SendCC(ableton_port, 15, (111 + track), 127);
              	}
              
            	}           

							// send a global clip change(all clips in the clip line)	
							// send play envet to ableton cc 20 with value 127 to channel 16
							//MIOS32_MIDI_SendCC(ableton_port, 15, 20, 127);

						}
						
						//
						// Autosave Pattern
						//
						if (auto_save) {
							
							// Autosave all 4 group of patterns
							for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
								
								// Save to SD Card
								if( (status=SEQ_PATTERN_Save(group, seq_pattern[group])) < 0 ) {
									SEQ_UI_SDCardErrMsg(2000, status);
								}
								
							}
							
							// Autosave Mixer Map
							SEQ_MIXER_Save(SEQ_MIXER_NumGet());
							
						}							
						
						// if we are not in remix mode 
						if (seq_pattern_remix_map == 0) {
							
							// keep the name of old pattern(because we still got 1 or more tracks belongs to this pattern)
							//pattern_name = seq_pattern_name;
							// set timer for mixed pattern
							pattern_remix_timer = MIOS32_SYS_TimeGet();
						}								
						
						//
						// Pattern change request
						//
            for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {

              // change pattern
              //selected_pattern[group].num = button-8;
              SEQ_PATTERN_Change(group, selected_pattern[group], 0);

							//if ( remix_mode ) {
								// keep the pattern name...
								//sprintf(seq_pattern_name[group], pattern_name);
							//}
							
            }
						
						if ( !remix_mode ) {
							// copy the pattern name for future reference
							//sprintf(pattern_name, seq_pattern_name[0]);	
							SEQ_PATTERN_PeekName(selected_pattern[0], pattern_name);
							// TODO: sync this with the pattern change handled by SEQ_PATTERN_Handler()
							//pattern_timer.seconds = 0;		
							preview_mode = 0;
						}
						
						// support for demix
            seq_pattern_demix_map = seq_pattern_remix_map;

            //preview_mode = 0;
						
          // Request a Pattern Preview
          } else {
            preview_num = button-8;
            SEQ_PATTERN_PeekName(selected_pattern[0], preview_name);

        	  // for security reasons in live environment, we can not accept 2x rapid press of pattern button.
      	    // if the buttons are not good quality they can send a 2x rapid signal os pressing in just one physical pressing
    	      // the time we need is bassicly the human time to read the name of patterns requested
  	        // start time in count
	          pc_time_control = MIOS32_TIMESTAMP_Get() + 200;

          }

        }			
				
        return 1; // value always changed
      }

      u8 visible_track = SEQ_UI_VisibleTrackGet();

      switch( button ) {

        case SEQ_UI_BUTTON_Right:
          // switch to next track
          if( visible_track < (SEQ_CORE_NUM_TRACKS - 1) )
            visible_track++;
					
          ui_selected_tracks = (1 << visible_track);
          ui_selected_group = visible_track / 4;

          return 1; // value always changed

        case SEQ_UI_BUTTON_Left:
          // switch to previous track
          if( visible_track >= 1 )
            visible_track--;
					
          ui_selected_tracks = (1 << visible_track);
          ui_selected_group = visible_track / 4;

          return 1; // value always changed

        case SEQ_UI_BUTTON_Up:
          return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

        case SEQ_UI_BUTTON_Down:
          return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
      }

      return -1; // invalid or unsupported button

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // Page do not exist
    default:
      break;

  }
	
	return 0;

}

/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  // layout:
  // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
  // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
  // <--------------------------------------> <-------------------------------------->
  //     No Patterns available as long as the Session hasn't been created!
  //                   Please press EXIT and  create a new Session!
	
  // Main Page
  // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
  // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
  // <--------------------------------------> <-------------------------------------->
  // A1: Breakbeats 2                   16.16 A4: Breakbeat full              10:10:10
  // ____ ____ ____ ____ T01: Track Name      x__x __x_ ____ ____              126 BPM
	
  // Remix Page
  // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
  // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
  // <--------------------------------------> <-------------------------------------->
  //  1    2    3    4    5    6    7    8     9    10   11   12   13   14   15   16
  // MIX  .... UMIX .... UMIX .... .... MIX   .... .... .... .... .... .... .... ....
	
  // In preview_mode we can state tracks to Mix state only(on/off), on normal mode
  // we can state track do DMix state only(on/off)
	
  // Options Page
  // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
  // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
  // <--------------------------------------> <-------------------------------------->
  // A1: Breakbeats 2                         Auto save Auto rset Abtn. Api
  // Save Name Copy Paste          Trk Delay     Off       On        On
	
  // Edit Name Page
  // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
  // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
  // <--------------------------------------> <-------------------------------------->
  // A1: <xxxxxxxxxxxxxxxxxxxx>
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8 WXYZ9 -_ 0  Char <>  Del Ins        OK  
	
  // Track Delay Page
  // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
  // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
  // <--------------------------------------> <-------------------------------------->
  //   1    2    3    4    5    6    7    8     9    10   11   12   13   14   15   16
  //  -10 +230  -45  -22  +23  -43 +221  -32  -210 +230  -45 -322 +323  -43 +221  -32
  // for this we could use t->bpm_tick_delay, but we need a way to saving
  // this values inside the track pattern

  if( SEQ_FILE_FormattingRequired() ) {
    if( high_prio )
      return 0;

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("    No Patterns available as long as theSession hasn't been created!            ");
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString("                  Please press EXIT and create a new Session!                   ");
    return 0;
  }
	
  switch( selected_page ) {

    ///////////////////////////////////////////////////////////////////////////
    // REMIX PAGE
    case PAGE_REMIX: {

      // Remix Page
      // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
      // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
      // <--------------------------------------><-------------------------------------->
      //  1M   2    3M   4    5    6    7    8    9    10   11   12   13   14   15   16
      // ...  .... .... .... .... .... .... .... .... .... .... .... .... .... .... ....

      // <--------------------------------------><-------------------------------------->
      //  1D    2   3D    4   5    6    7    8    9    10   11   12   13   14   15   16
      //  .... .... .... .... .... .... .... .... .... .... .... .... .... .... .... ....

      if( high_prio ) {

        ///////////////////////////////////////////////////////////////////////////
        // frequently update VU meters and step position counter
        u8 track;
        //u8 spacer = 0;

        SEQ_LCD_CursorSet(0, 1);

        seq_core_trk_t *t = &seq_core_trk[0];
        for(track=0; track<16; ++t, ++track) {

          //if( !(track % 4) && (track!=0) )
          //  spacer++;

          //SEQ_LCD_CursorSet(track+spacer, 1);


					//SEQ_LCD_PrintVBar(t->vu_meter >> 4);
					SEQ_LCD_PrintHBar(t->vu_meter >> 3);

        }

      } else { // not high_prio

        u8 track;

        SEQ_LCD_CursorSet(0, 0);
				SEQ_LCD_PrintSpaces(1);
				
        for(track=0; track<16; ++track) {

					// print the mixed state info
					if( seq_pattern_remix_map & (1 << track) ) {
						
						SEQ_LCD_PrintFormattedString("%dM", track+1);
            if (track < 9) {
              SEQ_LCD_PrintSpaces(3);
            } else {
              SEQ_LCD_PrintSpaces(2);
            }
						
					// print the demixed state info	
					} else if ( ( seq_pattern_remix_map ^ seq_pattern_demix_map ) & (1 << track) ) { 
					
						SEQ_LCD_PrintFormattedString("%dD", track+1);
            if (track < 9) {
              SEQ_LCD_PrintSpaces(3);
            } else {
              SEQ_LCD_PrintSpaces(2);
            }
						
				  }else {
						
            SEQ_LCD_PrintFormattedString("%d", track+1);
            if (track < 9) {
              SEQ_LCD_PrintSpaces(4);
            } else {
              SEQ_LCD_PrintSpaces(3);
            }
						
					}
					
        }
				
				SEQ_LCD_PrintSpaces(1);

      }

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // OPTION PAGE
    case PAGE_OPTION: {

      // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
      // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
      // <--------------------------------------> <-------------------------------------->
      // A1: Breakbeats 2                                              Auto save Abtn. Api
      // Save Name Copy Paste                                             Off        On

      ///////////////////////////////////////////////////////////////////////////
      // Current Pattern Name
			SEQ_LCD_Clear();
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintPattern(seq_pattern[0]);
      SEQ_LCD_PrintFormattedString(": %s", seq_pattern_name[0]);

      ///////////////////////////////////////////////////////////////////////////
      // Options Labels Left Side
      SEQ_LCD_CursorSet(0, 1);
      SEQ_LCD_PrintString("Save ");
      SEQ_LCD_PrintString("Name ");
      SEQ_LCD_PrintString("Copy ");
      SEQ_LCD_PrintString("Paste");

      //SEQ_LCD_CursorSet(30, 1);
      //SEQ_LCD_PrintString("Trk Delay ");

      ///////////////////////////////////////////////////////////////////////////
      // Options Labels Right Side
      SEQ_LCD_CursorSet(61, 0);
      SEQ_LCD_PrintString("Auto save ");
      //SEQ_LCD_PrintString("Auto rset ");
      SEQ_LCD_PrintString("Abtn. Api ");

      SEQ_LCD_CursorSet(61, 1);
      if (auto_save) {
        SEQ_LCD_PrintString("   On     ");
      } else {
        SEQ_LCD_PrintString("   Off    ");
      }

      //if (auto_reset) {
      //  SEQ_LCD_PrintString("   On     ");
      //} else {
      //  SEQ_LCD_PrintString("   Off    ");
      //}

      if (ableton_api) {
        SEQ_LCD_PrintString("   On     ");
      } else {
        SEQ_LCD_PrintString("   Off    ");
      }

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // PATTERN NAME EDIT PAGE
    case PAGE_NAME_EDIT: {

      u8 i;

			SEQ_LCD_Clear();
			
			SEQ_UI_KeyPad_LCD_Msg();
      SEQ_LCD_CursorSet(76, 1);
      SEQ_LCD_PrintString("Back");
			
			// Print the pattern name
      SEQ_LCD_CursorSet(0, 0);
      SEQ_LCD_PrintPattern(seq_pattern[0]);
      SEQ_LCD_PrintString(": ");

      for(i=0; i<20; ++i)
        SEQ_LCD_PrintChar(seq_pattern_name[ui_selected_group][i]);

      // insert flashing cursor
      if( ui_cursor_flash ) {
        SEQ_LCD_CursorSet(3 + ((ui_edit_name_cursor < 5) ? 1 : 2) + ui_edit_name_cursor, 0);
        SEQ_LCD_PrintChar('*');
      }

      //return 0;

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // TRACK DELAY PAGE
    case PAGE_TRK_DELAY: {

      // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
      // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
      // <--------------------------------------> <-------------------------------------->
      //   1    2    3    4    5    6    7    8     9    10   11   12   13   14   15   16
      //  -10 +230  -45  -22  +23  -43 +221  -32  -210 +230  -45 -322 +323  -43 +221  -32

      u8 track;

      SEQ_LCD_CursorSet(0, 2);
      for(track=0; track<16; ++track) {

          SEQ_LCD_PrintFormattedString("%d", track);
          if (track < 11) {
            SEQ_LCD_PrintSpaces(4);
          } else {
            SEQ_LCD_PrintSpaces(3);
          }

      }

      SEQ_LCD_CursorSet(1, 2);
      for(track=0; track<16; ++track) {

          SEQ_LCD_PrintFormattedString("%d", track);
          //SEQ_LCD_PrintFormattedString("%d", 0);
          if (track < 11) {
            SEQ_LCD_PrintSpaces(4);
          } else {
            SEQ_LCD_PrintSpaces(3);
          }

      }

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // MAIN PAGE
    case PAGE_MAIN: {

      // 0000000000111111111122222222223333333333 0000000000111111111122222222223333333333
      // 0123456789012345678901234567890123456789 0123456789012345678901234567890123456789
      // <--------------------------------------> <-------------------------------------->
      // A1: Breakbeats 2                   16.16 A4: Breakbeat full              10:10:10
      // ____ ____ ____ ____ T01: Track Name      x__x __x_ ____ ____              126 BPM

			//SEQ_LCD_CursorSet(0, 0);
			//SEQ_LCD_PrintSpaces();
			
      if( high_prio ) {

        ///////////////////////////////////////////////////////////////////////////
        // Frequently update VU meters
        u8 track;
        u8 spacer = 0;
        seq_core_trk_t *t = &seq_core_trk[0];
        for(track=0; track<16; ++t, ++track) {

          if( !(track % 4) && (track!=0) )
            spacer++;

          SEQ_LCD_CursorSet(track+spacer, 1);

          //if( seq_core_trk_muted & (1 << track) )
          //  SEQ_LCD_PrintVBar('M');
          //else
            SEQ_LCD_PrintVBar(t->vu_meter >> 4);

				}
				
				if ( remix_mode && preview_mode ) {
					
					spacer = 0;
					seq_core_trk_t *t = &seq_core_trk[0];
          for(track=0; track<16; ++t, ++track) {
						
            if( !(track % 4) && (track!=0) )
              spacer++;
						
            SEQ_LCD_CursorSet((track+spacer)+40, 1);
						
						if( !( seq_pattern_remix_map & (1 << track) ) )
							SEQ_LCD_PrintVBar(t->vu_meter >> 4);
						
          }
					
				}
				
				///////////////////////////////////////////////////////////////////////////
				// Print Pattern Relative Sequencer Position
				// SEQ_LCD_CursorSet(28, 0);
				// u32 tick = SEQ_BPM_TickGet();
				// tick = tick - seq_pattern_start_tick;
				// u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
				// u32 ticks_per_measure = ticks_per_step * (seq_core_steps_per_measure+1);
				// u32 measure = (tick / ticks_per_measure) + 1;
				// u32 step = ((tick % ticks_per_measure) / ticks_per_step) + 1;
				//u32 microstep = tick % ticks_per_step;
				//SEQ_LCD_PrintFormattedString("%8u.%3d.%3d", measure, step, microstep);
				// SEQ_LCD_PrintFormattedString("%8u.%3d", measure, step);

      } else { // not high_prio

				SEQ_LCD_Clear();
				
        ///////////////////////////////////////////////////////////////////////////
        // Current Pattern Name
        SEQ_LCD_CursorSet(0, 0);
        SEQ_LCD_PrintPattern(seq_pattern[0]);
				if (remix_mode == 1) {
					SEQ_LCD_PrintFormattedString(": %s", pattern_name);
				} else {
					SEQ_LCD_PrintFormattedString(": %s", seq_pattern_name[0]);
				}

				//
				
        ///////////////////////////////////////////////////////////////////////////
        // Print Selected Track Name
        SEQ_LCD_CursorSet(20, 1);
        u8 visible_track = SEQ_UI_VisibleTrackGet();
        if ( visible_track < 9 ) {
          SEQ_LCD_PrintFormattedString("T0%d: ", visible_track+1);
        } else {
          SEQ_LCD_PrintFormattedString("T%d: ", visible_track+1);
        }
        SEQ_LCD_PrintTrackLabel(visible_track, (char *)seq_core_trk[visible_track].name);
        //SEQ_LCD_PrintString((char *)seq_core_trk[visible_track].name);
				//SEQ_LCD_PrintSpaces();
		  
		  
        ///////////////////////////////////////////////////////////////////////////
        // Print Total Play Time
        SEQ_LCD_CursorSet(72, 0);
				if ( seq_play_timer.seconds == 0 ) {
					SEQ_LCD_PrintFormattedString("00:00:00");
				} else {
					mios32_sys_time_t play_time = MIOS32_SYS_TimeGet();
					play_time.seconds = play_time.seconds - seq_play_timer.seconds;
					int hours = (play_time.seconds / 3600) % 24;
					int minutes = (play_time.seconds % 3600) / 60;
					int seconds = (play_time.seconds % 3600) % 60;
					SEQ_LCD_PrintFormattedString("%02d:%02d:%02d", hours, minutes, seconds);
				}

				///////////////////////////////////////////////////////////////////////////
				// Print Total Play Time of Pattern
				SEQ_LCD_CursorSet(35, 0);
				
				if ( seq_play_timer.seconds == 0 ) {
					SEQ_LCD_PrintFormattedString("00:00");
					pattern_remix_timer = MIOS32_SYS_TimeGet();
				} else {
					mios32_sys_time_t play_time = MIOS32_SYS_TimeGet();
					play_time.seconds = play_time.seconds - pattern_remix_timer.seconds;
					int minutes = (play_time.seconds % 3600) / 60;
					int seconds = (play_time.seconds % 3600) % 60;
					SEQ_LCD_PrintFormattedString("%02d:%02d", minutes, seconds);	
				}
				
        ///////////////////////////////////////////////////////////////////////////
        // Print BPM
        SEQ_LCD_CursorSet(71, 1);
        float bpm = seq_core_bpm_preset_tempo[seq_core_bpm_preset_num];
        SEQ_LCD_PrintFormattedString("%3d.%d BPM", (int)bpm, (int)(10*bpm)%10);

        ///////////////////////////////////////////////////////////////////////////
        // Print Total Sequencer Position
        //SEQ_LCD_CursorSet(64, 1);
        //u32 tick = SEQ_BPM_TickGet();
        //u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
        //u32 ticks_per_measure = ticks_per_step * (seq_core_steps_per_measure+1);
        //u32 measure = (tick / ticks_per_measure) + 1;
        //u32 step = ((tick % ticks_per_measure) / ticks_per_step) + 1;
        //u32 microstep = tick % ticks_per_step;
        //SEQ_LCD_PrintFormattedString("%8u.%3d.%3d", measure, step, microstep);

        ///////////////////////////////////////////////////////////////////////////
        // Requested Pattern Name
        if ( preview_mode ) {

          SEQ_LCD_CursorSet(40, 0);
          //SEQ_LCD_PrintPatternLabel(preview_pattern, preview_pattern.name);
          //SEQ_LCD_PrintString(">>>");
          //SEQ_LCD_PrintFormattedString("Pattern N: %d, B: %d", preview_name, preview_bank);
          SEQ_LCD_PrintPattern(selected_pattern[0]);
          SEQ_LCD_PrintFormattedString(": %s", preview_name);
					
					if ( remix_mode ) {
						///////////////////////////////////////////////////////////////////////////
						// Print Total Play Time of Pattern remixed
						SEQ_LCD_CursorSet(65, 0);
						mios32_sys_time_t play_time = MIOS32_SYS_TimeGet();
						play_time.seconds = play_time.seconds - seq_pattern_start_time.seconds;
						int minutes = (play_time.seconds % 3600) / 60;
						int seconds = (play_time.seconds % 3600) % 60;
						SEQ_LCD_PrintFormattedString("%02d:%02d", minutes, seconds);
						
						/*
						
						if ( seq_play_timer.seconds == 0 ) {
							SEQ_LCD_PrintFormattedString("00:00");
							pattern_remix_timer = MIOS32_SYS_TimeGet();
						} else {
							mios32_sys_time_t play_time = MIOS32_SYS_TimeGet();
							play_time.seconds = play_time.seconds - pattern_remix_timer.seconds;
							int minutes = (play_time.seconds % 3600) / 60;
							int seconds = (play_time.seconds % 3600) % 60;
							SEQ_LCD_PrintFormattedString("%02d:%02d", minutes, seconds);
						}
						 
						 */
					}
					
        }
				
				if ( remix_mode ) {
				
					SEQ_LCD_CursorSet(29, 0);
					SEQ_LCD_PrintFormattedString("REMIX");
				}

        ///////////////////////////////////////////////////////////////////////////
        // Remix matrix
        if (seq_pattern_remix_map != 0) {
			
					u8 track;	
					u8 spacer = 0;

          for(track=0; track<16; ++track) {

            if( !(track % 4) && (track!=0) )
              spacer++;

            SEQ_LCD_CursorSet((track+spacer)+40, 1);

            // print who is in remix or deremix state
            if( seq_pattern_remix_map & (1 << track) )
              SEQ_LCD_PrintVBar('X');
            else
              SEQ_LCD_PrintVBar('_');

          }

        }

      }

    } break;

    ///////////////////////////////////////////////////////////////////////////
    // Page do not exist
    default:
      break;

  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PATTERN_RMX_Init(u32 mode)
{
	
	// init with our preview reference with the actucal pattern values
	preview_num = seq_pattern[0].num;
	preview_bank = seq_pattern[0].group;
	//pattern_timer.seconds = 0;
	
	// clean our string memory space
	//sprintf(pattern_name_copypaste, "");

  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show vertical VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);
	
  // init selection patterns
  u8 group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
    selected_pattern[group] = seq_pattern[group];

  return 0; // no error
}
