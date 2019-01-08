#define MCU_SAMPLE_RATE  8000

#define MCU_N_PEAKS		6
#define MCU_N_SEQ_FRAMES 25

typedef struct {
	short frflags;
	unsigned char length;
	unsigned char rms;
	short ffreq[6];
	unsigned char fheight[6];
	unsigned char fwidth[6];          // width/4
}MCU_frame_t;

#ifndef USE_MCU_VOICES
#define MCU_VOICE_NHARMONIC_PEAKS 55
#define MCU_VOIVE_PITCH_BASE 269008
#define MCU_VOICE_PITCH_RANGE 2780
#define MCU_SPEED_FACTOR1 256
#define MCU_SPEED_FACTOR2 238
#define MCU_SPEED_FACTOR3 232

#define MCU_VOICE_ROUGHNESS 1
#define MCU_VOICE_FLUTTER 64

#define MCU_espeakEVENT_WORD 1
#endif
