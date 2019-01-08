This is the MicroVGA module for MIOS32

Installation
--------------------------------------------------------------------------------

To use this module, just link microvga.mk to your MIOS32 project


Hardware
--------------------------------------------------------------------------------
Connect MicroVGA-TEXT to your MIOS32 as follows and set the jumper to 5V:

1 GND  -> J19:Vs
2 +5V  -> J19:5V
3 +3V3 NOT CONNECTED 
4 CS#  -> J19:RC1
5 SCK  -> J19:SC
6 RDY# -> J19:RC2
7 MISO -> J19:SI
8 MOSI -> J19:SO

