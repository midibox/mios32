// $Id$
/*
 * ==========================================================================
 * Copyright 2014 Mark Owen
 * http://www.quinapalus.com
 * E-mail: goom@quinapalus.com
 * 
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this file.  If not, see <http://www.gnu.org/licenses/> or
 * write to the Free Software Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <math.h>

#include "synth.h"

#include <FreeRTOS.h>
#include <portmacro.h>


#define NPOLY 16 // polyphony: must be a power of 2
#define NCHAN 16 // number of MIDI channels/patches: must be a power of 2


volatile int tbuf[4][2]; // L,R samples being prepared

const short sintab[256]={ // sine table, linearly interpolated by oscillators:
// In Octave:
// a=round(sin(2*pi*([-63 -63:1:63 63]/252))*32767)
// reshape([a'(1:128) a'(2:129)-a'(1:128)]',1,256)

  -32767,0,-32767,10,-32757,31,-32726,51,-32675,71,-32604,91,-32513,112,-32401,132,-32269,152,-32117,172,
  -31945,191,-31754,212,-31542,231,-31311,250,-31061,270,-30791,289,-30502,308,-30194,327,-29867,345,-29522,364,
  -29158,381,-28777,400,-28377,417,-27960,435,-27525,452,-27073,468,-26605,485,-26120,502,-25618,517,-25101,533,
  -24568,548,-24020,563,-23457,578,-22879,592,-22287,606,-21681,619,-21062,632,-20430,645,-19785,657,-19128,670,
  -18458,680,-17778,692,-17086,703,-16383,712,-15671,722,-14949,732,-14217,740,-13477,749,-12728,757,-11971,764,
  -11207,771,-10436,778,-9658,783,-8875,790,-8085,794,-7291,798,-6493,803,-5690,806,-4884,810,-4074,811,
  -3263,814,-2449,816,-1633,816,-817,817,0,817,817,816,1633,816,2449,814,3263,811,4074,810,
  4884,806,5690,803,6493,798,7291,794,8085,790,8875,783,9658,778,10436,771,11207,764,11971,757,
  12728,749,13477,740,14217,732,14949,722,15671,712,16383,703,17086,692,17778,680,18458,670,19128,657,
  19785,645,20430,632,21062,619,21681,606,22287,592,22879,578,23457,563,24020,548,24568,533,25101,517,
  25618,502,26120,485,26605,468,27073,452,27525,435,27960,417,28377,400,28777,381,29158,364,29522,345,
  29867,327,30194,308,30502,289,30791,270,31061,250,31311,231,31542,212,31754,191,31945,172,32117,152,
  32269,132,32401,112,32513,91,32604,71,32675,51,32726,31,32757,10,32767,0,
  };

// product of the following two tables is exp_2 of 12-bit fraction in Q30
const unsigned short exptab0[64]={ // "top octave generator": round(2^15*(2.^([0:1:63]/64)))
  32768,33125,33486,33850,34219,34591,34968,35349,35734,36123,36516,36914,37316,37722,38133,38548,
  38968,39392,39821,40255,40693,41136,41584,42037,42495,42958,43425,43898,44376,44859,45348,45842,
  46341,46846,47356,47871,48393,48920,49452,49991,50535,51085,51642,52204,52773,53347,53928,54515,
  55109,55709,56316,56929,57549,58176,58809,59449,60097,60751,61413,62081,62757,63441,64132,64830};

const unsigned short exptab1[64]={ // fine tuning: round(2^15*(2.^([0:1:31]/1024)))
  32768,32774,32779,32785,32790,32796,32801,32807,32812,32818,32823,32829,32835,32840,32846,32851,
  32857,32862,32868,32874,32879,32885,32890,32896,32901,32907,32912,32918,32924,32929,32935,32940,
  32946,32952,32957,32963,32968,32974,32979,32985,32991,32996,33002,33007,33013,33018,33024,33030,
  33035,33041,33046,33052,33058,33063,33069,33074,33080,33086,33091,33097,33102,33108,33114,33119};

unsigned char chup[NCHAN];     // channel controls updated?
unsigned char sus[NCHAN];      // sustain pedal position
unsigned short knob[24];       // raw 10-bit MSB-justified ADC results
unsigned char ctrl[NCHAN][24]; // 7-bit control values
short pbend[NCHAN];            // pitch bend position

struct egparams { // envelope generator parameters
  unsigned short a,d,s,r;
  };

struct egvars { // envelope generator variables
  unsigned short state;   // 0=off, 1=attacking, 2=holding, 3=decaying, 4=sustaining, 5=releasing
  unsigned short logout;  // logarithmic output 4Q12
  unsigned short out;     // linear output Q16
  };

// internal state in each voice
// chars then shorts then ints reduces average size of ldr:s with constant offsets
struct voicedata {
  unsigned char fk;                 // 0   filter constant
  signed char note;                 // 1   note number; b7: note is down
  signed char chan;                 // 2   channel to which this voice is allocated
  unsigned char vel;                // 3   note on velocity

  unsigned short o0p;              // 4     oscillator 0 waveshape constants
  unsigned short o0k0,o0k1;        // 6,8
  unsigned short o1p;              // 10    oscillator 1 waveshape constants
  unsigned short o1k0,o1k1;        // 12,14
  unsigned short o1egstate;        // 16    oscillator 1 envelope generator state
  unsigned short o1eglogout;       // 18    oscillator 1 envelope generator logarithmic output

  unsigned short vol;              // 20    current output level, copied from amplitude envelope generator output on zero-crossings
  unsigned short eg0trip;          // 22    count of update cycles since volume update
  struct egvars egv[2];            // 24    variables for amplitude and filter envelope generators

  unsigned short lvol,rvol;        // 36,38 multipliers for L and R outputs
  unsigned short o1egout;          // 40    oscillator 1 envelope generator linear output
  unsigned short o1vol;            // 42    current oscillator 1 level, copied from amplitude envelope generator output on zero-crossings

  int o0ph;                        // 44    oscillator 0 phase accumulator
  int o1ph;                        // 48    oscillator 1 phase accumulator
  int o0dph;                       // 52    oscillator 0 phase increment
  int o1dph;                       // 56    oscillator 1 phase increment

  int o1fb;                        // 60    oscillator 1 feedback value
  int lo,ba;                       // 64,68 filter state
  int out;                         // 72    last output sample from this voice

  int o1o;                         // 76    oscillator 1 output
  } vcs[NPOLY];

struct patchdata {
  unsigned short o1ega,o1egd;      // 0,2   oscillator 1 envelope generator parameters
  unsigned short o1vol;            // 4     oscillator 1 output level
  unsigned short lvol,rvol;        // 6,8   pan values
  unsigned char cut;               // 10    filter cutoff
  signed char fega;                // 11    filter sensitivity
  unsigned char res;               // 12    filter resonance
  unsigned char omode;             // 13    oscilaator mode: 0=mix 1=FM 2=FM+FB
  struct egparams egp[2];          // 14,22 parameters for amplitude and filter envelope generators
  } patch[NCHAN];

// external assembler routines
extern void wavupa(); // waveform generation code


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// at 48kHz sample frequency and two channels, the sample buffer has to be refilled
// at a rate of 48Khz / SAMPLE_BUFFER_SIZE
#define SAMPLE_BUFFER_SIZE 8  // -> 8 L/R samples, 6 kHz refill rate (166 uS period)


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// sample buffer
static u32 sample_buffer[SAMPLE_BUFFER_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

void SYNTH_ReloadSampleBuffer(u32 state);


/////////////////////////////////////////////////////////////////////////////
// initializes the synth
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_Init(u32 mode)
{
  // use J10A.D0 for performance measurements
  MIOS32_BOARD_J10_PinInit(0, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);

  // start I2S DMA transfers
  return MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &SYNTH_ReloadSampleBuffer);
}

/////////////////////////////////////////////////////////////////////////////
// derive frequency and volume settings from controller values for one voice
/////////////////////////////////////////////////////////////////////////////
static void setfreqvol(struct voicedata*v,unsigned char*ct) {
  int u,l;
  unsigned int f;
  int p,q,p0,p1,p2;

  struct patchdata*pa=patch+v->chan;

  // oscillator 0 frequency
  u=((v->note&0x7f)<<12)/12; // pitch of note, Q12 in octaves, middle C =5
  u+=pbend[v->chan]/12;      // gives +/- 2 semitones
  u-=287; // constant to give correct tuning for sample rate: log((72e6/2048)/(440*2^-0.75)/128)/log(2)*4096 for A=440Hz
  f=(exptab0[(u&0xfc0)>>6]*exptab1[u&0x3f])>>(10-(u>>12)); // convert to linear frequency
  v->o0dph=f;

  // oscillator 0 waveform
  l=f>>13; // compute slope limit: l=32768 at 1/8 Nyquist
  if(l>30000) l=30000;  // keep within sensible range
  if(l<1024) l=1024;

  // waveform has four periods p0-3: slope0, flat0, slope1, flat1
  p=0x8000-(ct[16]*248); // first half p=[1272,32768]
  q=0x10000-p;           // second half q=[32768,64264]
  p0=(p*(127-ct[17]))>>7;
  if(p0<l) p0=l; // limit waveform slope
  p1=p-p0;
  p2=(q*(127-ct[17]))>>7;
  if(p2<l) p2=l;
//  p3=q-p2; // not used
  v->o0p=(p0+p2)/2+p1; // constants used by assembler code
  v->o0k0=0x800000/p0;
  v->o0k1=0x800000/p2;

  // oscillator 1 frequency
  if(ct[7]>0x60) u=-0x1000-287;     // fixed "low" frequency
  else if(ct[7]>0x20) u=0x3000-287; // fixed "high" frequency
  u+=(ct[2]<<7)+(ct[3]<<3)-0x2200;
  f=(exptab0[(u&0xfc0)>>6]*exptab1[u&0x3f])>>(10-(u>>12));
  v->o1dph=f;

  // oscillator 1 waveform
  l=f>>13; // =32768 at 1/8 Nyquist
  if(l>30000) l=30000;
  if(l<1024) l=1024;

  p=0x8000-(ct[0]*248); // first half p=[1272,32768]
  q=0x10000-p;          // second half q=[32768,64264]
  p0=(p*(127-ct[1]))>>7;
  if(p0<l) p0=l;
  p1=p-p0;
  p2=(q*(127-ct[1]))>>7;
  if(p2<l) p2=l;
//  p3=q-p2; // not used
  v->o1p=(p0+p2)/2+p1;
  v->o1k0=0x800000/p0;
  v->o1k1=0x800000/p2;

  v->lvol=(pa->lvol*v->vel)>>7; // calculate output multipliers taking velocity into account
  v->rvol=(pa->rvol*v->vel)>>7;
  }

/////////////////////////////////////////////////////////////////////////////
// process all controllers for given channel
/////////////////////////////////////////////////////////////////////////////
static void procctrl(int chan) {
  struct patchdata*pa=patch+chan;
  unsigned char*ct=ctrl[chan];
  int i;

  i=ct[6];
  if(i) i=(exptab0[(i&0xf)<<2])>>(7-(i>>4)); // convert oscillator 1 level to linear
  pa->o1vol=i;

  pa->o1ega=0xffff/(ct[4]*ct[4]/16+1);  // scale oscillator 1 eg parameters
  if(ct[5]==127) pa->o1egd=0;
  else           pa->o1egd=0xffff/(ct[5]*ct[5]+1);

  pa->egp[0].a=0xffff/(ct[12]*ct[12]/16+1);  // scale amplitude eg parameters
  pa->egp[0].d=0xffff/(ct[13]*ct[13]+1);
  if(ct[14]==0) pa->egp[0].s=0;
  else          pa->egp[0].s=0xc000+(ct[14]<<7);
  pa->egp[0].r=0xffff/(ct[15]*ct[15]+1);

  pa->egp[1].a=0xffff/(ct[8]*ct[8]/16+1);  // scale filter eg parameters
  pa->egp[1].d=0xffff/(ct[9]*ct[9]+1);
  i=ct[10]; // sustain level
  if(i) i=exptab0[(i&0xf)<<2]>>(7-(i>>4));
  pa->egp[1].s=i;
  pa->egp[1].r=0xffff/(ct[11]*ct[11]+1);

  pa->cut=ct[20]<<1;   // scale filter control parameters
  pa->fega=(ct[19]<<1)-128;
  pa->res=0xff-(ct[21]<<1);

  if(ct[18]<0x20) pa->omode=0;  // oscillator combine mode
  else if(ct[18]>0x60) pa->omode=2;
  else pa->omode=1;

  i=ct[22]; // volume
  if(i) i=exptab0[(i&0xf)<<2]>>(7-(i>>4)); // convert to linear
  pa->lvol=(sintab[254-(ct[23]&~1)]*i)>>15; // apply pan settings maintining constant total power
  pa->rvol=(sintab[128+(ct[23]&~1)]*i)>>15;
  for(i=0;i<NPOLY;i++) if(vcs[i].chan==chan) setfreqvol(vcs+i,ct); // update any affected voices
  }


/////////////////////////////////////////////////////////////////////////////
// silence one voice
/////////////////////////////////////////////////////////////////////////////
void stopv(struct voicedata*v) {
  MIOS32_IRQ_Disable();

  v->egv[0].state=0; // stop aeg processing for voice
  v->egv[0].logout=0;
  v->egv[0].out=0;   // silence voice
  v->egv[1].state=0; // stop feg processing for voice
  v->egv[1].logout=0;
  v->egv[1].out=0;
  v->eg0trip=0;
  v->o1egstate=1;
  v->o1eglogout=0;
  v->o1egout=0;
  v->o1vol=0;
  v->o1o=0;
  v->o1fb=0;
  v->fk=0;
  v->chan=NCHAN-1;
  v->vol=0;
  v->out=0;
  v->o0ph=0x00000000;
  v->o0dph=0x00000000;
  v->o1dph=0x00000000;
  v->lo=v->ba=0;

  MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Handles incoming MIDI data
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  switch( midi_package.type ) {
  case NoteOff:
    midi_package.velocity = 0; // set velocity to 0 and fall through
  case NoteOn: {
    int mu=-1;
    int mi=0;
    int i;

    struct voicedata *v;
    for(i=0;i<NPOLY;i++) { // find a voice to use
      v = vcs+i;
      int u=v->egv[0].state;
      if(u&&v->chan==midi_package.chn&&(v->note&0x7f)==midi_package.note) break; // this channel+note combination already sounding? reuse the voice
      if(u==0) u=6; // preferentially steal an idle voice
      u=(u<<16)-v->egv[0].logout;
      if(u>mu) mu=u,mi=i; // find most advanced+quietest voice to steal
    }

    u8 retrig = 0;
    if(midi_package.velocity==0) { // note off
      if(i==NPOLY) break; // ignore if note already gone
      vcs[i].note&=0x7f;    // allow exit from attack/sustain
      break;
    } else {
      // here we have a note on message
      if(i<NPOLY) { // note already sounding?
	v=vcs+i;
	retrig = 1;
      }
    }
    if( !retrig ) {
      // note on, not already sounding:
      v=vcs+mi; // point to voice to be stolen
      stopv(v); // silence it
    }

    MIOS32_IRQ_Disable();
    v->note=midi_package.note|0x80;  // set up voice data
    v->chan=midi_package.chn;
    v->vel=midi_package.velocity;
    MIOS32_IRQ_Enable();
    setfreqvol(v,ctrl[midi_package.chn]);
    MIOS32_IRQ_Disable();
    v->egv[0].state=1; // trigger note
    v->egv[1].state=1;
    v->eg0trip=0;
    v->o1egstate=0;
    v->o1eglogout=0;
    v->o1egout=0;
    v->out=0;
    MIOS32_IRQ_Enable();
  } break;

  case CC: {
    if(midi_package.cc_number==120||midi_package.cc_number>=123) { // all notes off
      int i;
      for(i=0;i<NPOLY;i++) stopv(vcs+i);
      break;
    }
    if(midi_package.cc_number==64) { // sustain pedal
      sus[midi_package.chn]=midi_package.value;
      break;
    }
    if(midi_package.cc_number>= 16&&midi_package.cc_number< 32) { ctrl[midi_package.chn][midi_package.cc_number-16    ]=midi_package.value; chup[midi_package.chn]=1; } // controller 0..15; set update flag
    if(midi_package.cc_number>=102&&midi_package.cc_number<110) { ctrl[midi_package.chn][midi_package.cc_number-102+16]=midi_package.value; chup[midi_package.chn]=1; } // controller 16..23; set update flag
  } break;
  case PitchBend: {
    int i;
    pbend[midi_package.chn]=(midi_package.evnt2<<7)+midi_package.evnt1-0x2000;
    for(i=0;i<NPOLY;i++) if(vcs[i].chan==midi_package.chn) setfreqvol(vcs+i,ctrl[midi_package.chn]); // update frequencies for all affected voices
  } break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Called periodically to update channel controllers
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_Update_1mS(void)
{
  int i;

  for(i=0; i<NCHAN; ++i) {
    if( chup[i] ) {
      chup[i]=0;    // reset update flag
      procctrl(i);  // update next channel
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called by MIOS32_I2S when the lower (state == 0) or 
// upper (state == 1) range of the sample buffer has been transfered, so 
// that it can be updated
/////////////////////////////////////////////////////////////////////////////
void SYNTH_ReloadSampleBuffer(u32 state)
{
  static u32 h = 0; // interrupt counter

  // transfer new samples to the lower/upper sample buffer range
  int i;
  u32 *buffer = (u32 *)&sample_buffer[state ? (SAMPLE_BUFFER_SIZE/2) : 0];

  MIOS32_BOARD_J10_PinSet(0, 1); // for performance measurements at J10A.D0

  // update waves
  // on a STM32F407 this takes ca. 35 uS
  wavupa();

  // transfer into sample buffer
  {
    int *tbuf_ptr = (int *)&tbuf[0];
    for(i=0; i<SAMPLE_BUFFER_SIZE/2; ++i) {
      // 24bit -> 16bit data
      u16 chn1_value = *(tbuf_ptr++) >> 16;
      u16 chn2_value = *(tbuf_ptr++) >> 16;

      *buffer++ = ((u32)chn2_value << 16) | chn1_value;
    }
  }

  MIOS32_BOARD_J10_PinSet(0, 0); // for performance measurements at J10A.D0


  {
    ++h; // count interrupts

    struct voicedata *v = vcs+(h&(NPOLY-1)); // choose a voice for eg processing
    struct patchdata *p = patch+v->chan;     // find its parameters
    int n = !!(h&NPOLY);       // choose an eg
    struct egparams *ep = p->egp+n;         // get pointers to parameters and variables
    struct egvars *ev = v->egv+n;
    int i = ev->logout;
    int j = (v->note&0x80)||sus[v->chan]!=0; // note down?

    if(ev->state==0) i=0;
    else {
      if(!j) ev->state=5; // exit sustain when note is released
      switch(ev->state) {
      case 1:
	i+=ep->a;  // attack
	if(i>=0x10000) i=0xffff, ev->state=2;
	break;
      case 2:
	i--;       // hold at top of attack
	if(i<=0xfff0) ev->state=3; // hold for 16 iterations
	break;
      case 3:
	i-=ep->d;  // decay
	if(i<ep->s) i=ep->s, ev->state=4;
	break;
      case 4:  // sustain
	break;
      case 5:
	i-=ep->r;  // release
	if(i<0) i=0, ev->state=0;
	break;
      }
    }

    ev->logout=i;
    if(i==0) ev->out=0;
    else ev->out=(exptab0[(i&0xfc0)>>6]*exptab1[i&0x3f])>>(31-(i>>12)); // compute linear output

    if(n==0) { // do oscillator 1 eg as well
      i=v->eg0trip;
      if(i>4) v->vol=ev->out;
      if(v->vol==ev->out) i=0;
      i++;
      v->eg0trip=i;
      i=v->o1eglogout;
      if(!j) v->o1egstate=1;
      if(v->o1egstate==0) { // attack
	i+=p->o1ega;
	if(i>=0x10000) i=0xffff, v->o1egstate=1;
      } else { // decay
	i-=p->o1egd;
	if(i<0) i=0;
      }
      v->o1eglogout=i;
      if(i==0) v->o1egout=0;
      else v->o1egout=(((exptab0[(i&0xfc0)>>6]*exptab1[i&0x3f])>>(31-(i>>12)))*p->o1vol)>>16; // compute linear output
    } else { // recalculate filter coefficient
      int k=((p->cut*p->cut)>>8)+((v->egv[1].logout*((p->fega*v->vel)>>6))>>15);
      if(k<0) k=0;
      if(k>255) k=255;
      v->fk=k;
    }
  }

#if 0
  n=h&7; // determine A/D batch
  knob[n   ]=(short)ADC_DR0; // read three conversion results, one from each bank of 8 controls
  knob[n+ 8]=(short)ADC_DR1;
  knob[n+16]=(short)ADC_DR2;
#endif
}
