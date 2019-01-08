MBHP_Genesis Module
^^^^^^^^^^^^^^^^^^^

HARDWARE
========

You must use a level shifter circuit between CORE_STM32F4 and MBHP_Genesis! See
http://wiki.midibox.org/doku.php?id=mbhp_genesis_ls

The J10 connector on MBHP_Genesis is designed to correspond directly to J10A and
J10B on CORE_STM32F4 (so you can just concatenate the two 10-pin IDC cables from
J10A:J10B to run to J10, again through your level shifter!). Thus the mappings
are:

MBHP_Genesis:J10    CORE_STM32F4    STM32F4
-------------------------------------------
D0                  J10A:D0         PE8
D1                  J10A:D1         PE9
D2                  J10A:D2         PE10
D3                  J10A:D3         PE11
D4                  J10A:D4         PE12
D5                  J10A:D5         PE13
D6                  J10A:D6         PE14
D7                  J10A:D7         PE15
/CS                 J10B:D8         PC13
/RD                 J10B:D9         PC14
/WR                 J10B:D10        PC15
A0                  J10B:D11        PE2
A1                  J10B:D12        PE4
A2                  J10B:D13        PE5
A3                  J10B:D14        PE6
A4                  J10B:D15        PE7



SOFTWARE
========

Please note that this driver does not do any busy timing. Timing is done by the
VGM playback engine in the appropriate MIDIbox Genesis application. However, the
driver does do the timing within individual writes/reads (holding data on buses
for the right duration).

Also, please note that the chip data structures are provided so that
application code can read the current chip state. Writing to these data
structures will not cause the chips to be updated. To change the board
filter capacitor bits, write to genesis[board].board.cap_psg or .cap_opn2
and then call Genesis_WriteBoardBits(board)--other than this, you probably
shouldn't be writing to the genesis data structure.

Sample use cases of these data structures:
u8 is_ssg_toggle_active = genesis[3].opn2.chan[4].op[0].ssg_toggle;
u16 psg_freq = genesis[2].psg.square[1].freq;
Genesis_CheckPSGBusy(u8 board);
u8 got_timer_irq = genesis[0].board.opn2_irq;
