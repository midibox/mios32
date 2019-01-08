// $Id: mbfm.h $
/*
 * Header file for MBHP_MBFM MIDIbox FM V2.0 synth engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBFM_H
#define _MBFM_H

#ifndef OPL3_COUNT
#error "OPL3_COUNT undefined! Please set up OPL3 module in mios32_config.h!"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//This is the size of the notestack BEYOND those notes actually playing on OPL3 voices
#ifndef MBFM_NOTESTACK_LEN
#define MBFM_NOTESTACK_LEN 8
#endif

typedef union {
  u16 all;
  struct{
    u8 note:7;
    u8 update:1;
    u8 velocity:7;
    u8 retrig:1;
  };
} notevel_t;

typedef union {
  u8 all;
  struct{
    u8 autodupl:1;
    u8 autoload:1;
    u8 cc_bmsb:1;
    u8 cc_blsb:1;
    u8 cc_prog:1;
    u8 cc_pan:1;
    u8 cc_other:1;
    u8 dummy:1;
  };
} chanoptions_t;

typedef union {
  u8 all;
  struct{
    s8 voice:7;
    u8 follow:1;
  };
} dupllink_t;

typedef union {
  u8 all;
  struct{
    u8 level:7;
    u8 enable:1;
  };
} midivol_t;

typedef union {
  u8 all;
  struct{
    u8 chan:4;
    u8 mode:2;
    u8 dummy:2;
  };
} midiportinopts_t;

typedef union {
  u8 all;
  struct{
    u8 thruport:3;
    u8 fpcc:1;
    u8 seq:1;
    u8 dummy:3;
  };
} midiportoutopts_t;

typedef union {
  u8 all;
  struct{
    s8 activity:3;
    u8 refreshvol:1;
    u8 sustain:1;
    u8 dummy:3;
  };
} voicemisc_t;


extern void MBFM_Init();
extern void MBFM_InitAfterFrontPanel();
extern void MBFM_1msTick(u32 time); //Things that are fast and should be done every ms
extern void MBFM_BackgroundTick(u32 time); //Things that are slow and can wait
extern void MBFM_ReceiveMIDIMessage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern void MBFM_SetVoiceAndLinkedNote(u8 voice, u8 note, u8 velocity);
extern u8 MBFM_GetNumVoicesInChannel(u8 channel, u8 ops);
extern void MBFM_LetGoSustainPedal(u8 channel);
extern void MBFM_AllNotesOff(u8 channel);
extern s8 MBFM_CopyVoice(u8 srcvoice, u8 destvoice); //For DUPL and LINK, return -1 if voices not same #ops or something
extern void MBFM_AssignVoiceToChannel(u8 voice, u8 chan);

extern s8 midichanmapping[18*OPL3_COUNT];
extern s8 midifirstvoice[16];
extern u8 mididrummapping[6*OPL3_COUNT];

extern dupllink_t voicedupl[18*OPL3_COUNT];
extern dupllink_t voicelink[18*OPL3_COUNT];

extern notevel_t voicemidistate[18*OPL3_COUNT]; //.update used for sustain pedal
extern u8 voicectr[16];
extern notevel_t channotestacks[16][MBFM_NOTESTACK_LEN]; //.update used for sustain pedal
//Post-delay, pre-TP/tuning/porta
extern notevel_t voiceactualnotes[18*OPL3_COUNT]; //.update and .retrig used properly
extern midivol_t midivol[16];
extern midivol_t midiexpr[16];

extern midiportinopts_t midiportinopts[6];
extern midiportoutopts_t midiportoutopts[6];

extern u8 chansustainpedals[16];
extern chanoptions_t chanoptions[16];

extern s8 drumactivity[6*OPL3_COUNT];
extern voicemisc_t voicemisc[18*OPL3_COUNT];
extern s8 midiactivity[16];


#ifdef __cplusplus
}
#endif

#endif /* _MBFM_H */
