@ Copyright 2014 Mark Owen
@ http://www.quinapalus.com
@ E-mail: goom@quinapalus.com
@
@ This file is free software: you can redistribute it and/or modify
@ it under the terms of version 2 of the GNU General Public License
@ as published by the Free Software Foundation.
@
@ This file is distributed in the hope that it will be useful,
@ but WITHOUT ANY WARRANTY; without even the implied warranty of
@ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@ GNU General Public License for more details.
@
@ You should have received a copy of the GNU General Public License
@ along with this file.  If not, see <http://www.gnu.org/licenses/> or
@ write to the Free Software Foundation, Inc., 51 Franklin Street,
@ Fifth Floor, Boston, MA  02110-1301, USA.

.syntax unified
.cpu cortex-m3
.thumb

.global wavupa
.extern vcs
.extern patch
.extern sintab
.extern tbuf   @ sample buffer

.set NPOLY,16

@ structure member offsets
.set egv_out,      4

.set v_fk,         0
.set v_note,       1
.set v_chan,       2
.set v_vel,        3

.set v_o0p,        4
.set v_o0k0,       6
.set v_o0k1,       8
.set v_o1p,       10
.set v_o1k0,      12
.set v_o1k1,      14
.set v_o1egstate, 16
.set v_o1eglogout,18

.set v_vol,       20
.set v_egv0,      24
.set v_egv1,      30

.set v_lvol,      36
.set v_rvol,      38
.set v_o1egout,   40
.set v_o1vol,     42

.set v_o0ph,      44
.set v_o1ph,      48
.set v_o0dph,     52
.set v_o1dph,     56

.set v_o1fb,      60
.set v_lo,        64
.set v_ba,        68

.set v_out,       72
.set v_o1o,       76
.set sizeof_voicedata,80

.set p_o1ega,      0
.set p_o1egd,      2
.set p_o1vol,      4
.set p_lvol,       6
.set p_rvol,       8
.set p_cut,       10
.set p_fega,      11
.set p_res,       12
.set p_omode,     13
.set p_egp0,      14
.set p_egp1,      22
.set sizeof_patchdata,30

@ oscillator kernel
.macro oker
 sub r4,r8,r3          @ compute position in sine given two possible slopes
 mul r4,r4,r10
 mul r5,r3,r9
 cmp r4,r12            @ determine which slope is required
 it ge
 movge r4,r5
 cmn r4,r12
 it le
 suble r4,r5,r9,lsl#16
 ubfx r5,r4,#10,#6     @ extract fractional part needed for interpolation
 ssat r4,#7,r4,asr#16  @ clamp result to produce flat parts of waveform
 ldr r4,[r11,r4,lsl#2] @ fetch sine value and derivative
 sxth r3,r4            @ extract sine part
 muls r4,r4,r5         @ compute derivative term
 add r4,r3,r4,lsr#22   @ interpolated result
.endm


.macro osc @ plain oscillator
 adds r2,r2,r7          @ accumulate phase
 movs r3,r2,lsr#16      @ rescale
 oker
.endm

.macro oscfb @ with FM feedback
 adds r2,r2,r7          @ accumulate phase
 add r3,r2,r4,lsl#8     @ add in frequency (actually phase) modulation
 movs r3,r3,lsr#16      @ rescale
 oker
.endm

.macro oscfm @ with FM
 adds r2,r2,r7          @ accumulate phase
 add r3,r2,r4,lsl#4     @ add in frequency (actually phase) modulation
 movs r3,r3,lsr#16      @ rescale
 oker
.endm


@ R0=v
@ R1=p
wavupa:
 push {r4-r12,r14}  @ we will use all the registers
 sub r13,r13,#16    @ make room on stack

 ldr r0,=tbuf       @ clear output sample accumulator buffer
 movs r1,#0
 movs r2,#0
 movs r3,#0
 movs r4,#0
 stmia r0!,{r1-r4}
 stmia r0!,{r1-r4}

 ldr r0,=vcs         @ r0 points to the voice data
 movs r12,#0x400000  @ constant needed by oscillator kernel
wa17:
 ldrb r3,[r0,#v_chan]       @ get corresponding patch data for this voice: pointer in r1
 movs r2,#sizeof_patchdata
 ldr r1,=patch
 mla r1,r2,r3,r1

 ldr r2,[r0,#v_o1ph]    @ fetch oscillator 1 parameters
 ldr r7,[r0,#v_o1dph]
 ldrh r8,[r0,#v_o1p]
 ldrh r9,[r0,#v_o1k0]
 ldrh r10,[r0,#v_o1k1]
 ldr r11,=(sintab+256)
 ldrh r6,[r0,#v_o1vol]
 ldrb r14,[r1,#p_omode] @ switch on oscillator combine mode
 cmp r14,#1
 bgt wa0

@ omode=0,1 (mix, FM)
 osc                @ compute 4 samples with output scaled by r6
 muls r4,r4,r6
 str r4,[r13,#0]
 osc
 muls r4,r4,r6
 str r4,[r13,#4]
 osc
 muls r4,r4,r6
 str r4,[r13,#8]
 osc
 mul r3,r4,r6
 str r3,[r13,#12]
 b wa1

wa0:
@ omode=2 (FM+FB)
 ldr r4,[r0,#v_o1fb]  @ get feedback value
 oscfb                @ compute 4 samples with output scaled by r6
 muls r4,r4,r6
 str r4,[r13,#0]
 oscfb
 muls r4,r4,r6
 str r4,[r13,#4]
 oscfb
 muls r4,r4,r6
 str r4,[r13,#8]
 oscfb
 mul r3,r4,r6
 str r3,[r13,#12]
 str r3,[r0,#v_o1fb] @ store feedback value for next time
wa1:
 str r2,[r0,#v_o1ph]

 ldrh r6,[r0,#v_o1egout]
 ldr r3,[r0,#v_o1o]
 eors r3,r3,r4
 it mi
 strhmi r6,[r0,#v_o1vol] @ update vol at zero-crossing of output
 str r4,[r0,#v_o1o]

 ldr r2,[r0,#v_o0ph]  @ fetch oscillator 0 parameters
 ldr r7,[r0,#v_o0dph]
 ldrh r8,[r0,#v_o0p]
 ldrh r9,[r0,#v_o0k0]
 ldrh r10,[r0,#v_o0k1]
 cmp r14,#0
 bgt wa2

@ omode=0 (mix)
 osc               @ generate 4 samples, summing results into output
 ldr r3,[r13,#0]
 add r4,r4,r3,asr#14
 str r4,[r13,#0]
 osc
 ldr r3,[r13,#4]
 add r4,r4,r3,asr#14
 str r4,[r13,#4]
 osc
 ldr r3,[r13,#8]
 add r4,r4,r3,asr#14
 str r4,[r13,#8]
 osc
 ldr r3,[r13,#12]
 add r11,r4,r3,asr#14
 b wa3
wa2:
@ omode=1,2 (FM, FM+FB)
 ldr r4,[r13,#0]  @ generate 4 samples
 oscfm
 str r4,[r13,#0]
 ldr r4,[r13,#4]
 oscfm
 str r4,[r13,#4]
 ldr r4,[r13,#8]
 oscfm
 str r4,[r13,#8]
 ldr r4,[r13,#12]
 oscfm
 movs r11,r4
wa3:
 str r2,[r0,#v_o0ph]

 ldmia r13,{r8-r10}  @ load back samples (last one was not stored)

 ldrb r2,[r0,#v_fk]  @ fetch filter parameters and state
 ldrb r3,[r1,#p_res]
 ldr r4,[r0,#v_lo]
 ldr r5,[r0,#v_ba]

.macro filstep rx  @ filter step using sample in register rx
 mul r7,r2,r5      @ simple CSound-style second order filter
 movs r7,r7,asr#8
 muls r7,r2,r7
 add r4,r4,r7,asr#8

 mul r7,r3,r5
 sub r7,\rx,r7,asr#8
 subs r7,r7,r4 @ hi

 muls r7,r2,r7
 movs r7,r7,asr#8
 muls r7,r2,r7
 add r5,r5,r7,asr#8

 ssat \rx,#17,r4   @ clamp output
.endm

 filstep r8  @ run filter over 4 samples
 filstep r9
 filstep r10
 filstep r11

 str r4,[r0,#v_lo]  @ save internal filter state
 str r5,[r0,#v_ba]

 ldr r14,=tbuf
 ldrh r2,[r0,#v_lvol]
 ldrh r3,[r0,#v_rvol]
wa34:

 ldr r7,[r0,#v_out]
 str r11,[r0,#v_out]
 cbz r7,wa35
 teq r7,r8
 bmi waz0 @ immediate zero-crossing?

wa35:
@ otherwise start at amplitude stored in vol, possibly switch later to aeg output if a zero-crossing is found
 ldrh r4,[r0,#v_vol]
 muls r2,r2,r4
 movs r2,r2,lsr#16 @ scale by left and right amplitudes to get overall left and right multipliers
 muls r3,r3,r4
 movs r3,r3,lsr#16

 teq r8,r9     @ look for zero crossing within four samples
 bmi waz1
 teq r9,r10
 bmi waz2
 teq r10,r11
 bmi waz3

@ no zero-crossings within four samples
waz0r:
 ldmia r14,{r4-r7}  @ accumulate 2 scaled samples into output buffer
 mla r4,r2,r8,r4
 mla r5,r3,r8,r5
 mla r6,r2,r9,r6
 mla r7,r3,r9,r7
 stmia r14!,{r4-r7}
waz2r:
 ldmia r14,{r4-r7}  @ accumulate 2 scaled samples into output buffer
 mla r4,r2,r10,r4
 mla r5,r3,r10,r5
 mla r6,r2,r11,r6
 mla r7,r3,r11,r7
 stmia r14!,{r4-r7}
waz4r:
 adds r0,r0,#sizeof_voicedata   @ next voice
 ldr r1,=vcs+NPOLY*sizeof_voicedata
 cmp r0,r1
 bne wa17

 add r13,r13,#16   @ restore stack
 pop {r4-r12,r15}  @ return

.macro swvol0       @ switch volume to aeg output
 ldrh r4,[r0,#v_egv0+egv_out]
 strh r4,[r0,#v_vol]
 ldrh r2,[r0,#v_lvol]
 ldrh r3,[r0,#v_rvol]
 muls r2,r2,r4
 movs r2,r2,lsr#16 @ scale by left and right amplitudes to get overall left and right multipliers
 muls r3,r3,r4
 movs r3,r3,lsr#16
.endm

waz0:
 swvol0
 b waz0r

waz1:
 ldmia r14,{r4-r5}  @ process one sample
 mla r4,r2,r8,r4
 mla r5,r3,r8,r5
 stmia r14!,{r4-r5}
 swvol0             @ switch volume
 ldmia r14,{r4-r5}  @ process one more sample
 mla r4,r2,r9,r4
 mla r5,r3,r9,r5
 stmia r14!,{r4-r5}
 b waz2r            @ drop into code above to process remaining two samples

waz2:
 ldmia r14,{r4-r7}  @ process two samples
 mla r4,r2,r8,r4
 mla r5,r3,r8,r5
 mla r6,r2,r9,r6
 mla r7,r3,r9,r7
 stmia r14!,{r4-r7}
 swvol0             @ switch volume
 b waz2r            @ drop into code above to process remaining two samples

waz3:
 ldmia r14,{r4-r7}  @ process two samples
 mla r4,r2,r8,r4
 mla r5,r3,r8,r5
 mla r6,r2,r9,r6
 mla r7,r3,r9,r7
 stmia r14!,{r4-r7}
 ldmia r14,{r4-r5}  @ process one sample
 mla r4,r2,r10,r4
 mla r5,r3,r10,r5
 stmia r14!,{r4-r5}
 swvol0             @ switch volume
 ldmia r14,{r4-r5}  @ process one more sample
 mla r4,r2,r11,r4
 mla r5,r3,r11,r5
 stmia r14!,{r4-r5}
 b waz4r            @ drop into code above to process next voice

wavupaend:
