MBHP_OPL3 Module
^^^^^^^^^^^^^^^^^^^^^^^

Modified from MBHP_SID Module Mios32 code by Sauraen


Overview:
---------


Digital wiring:
---------------
OPL3's Reset line must be buffered to 5 volts; all the other inputs may be driven by 3.3 volts.
Pinout:
  OPL3 D7:0 -> PE15:8 (J10A D7:0)
  OPL3 A1:0 -> PE7:6 (J10B D15:14)

Define the following in your config file and override:
#ifndef OPL3_COUNT
#define OPL3_COUNT 1       //2
#endif

#ifndef OPL3_RS_PORT
#define OPL3_RS_PORT  GPIOB
#endif
#ifndef OPL3_RS_PIN
#define OPL3_RS_PIN   GPIO_Pin_8
#endif

//These are the J10 pin numbers
#ifndef OPL3_CS_PINS
#define OPL3_CS_PINS  {13} //{12,13}
#endif
//These are 1s shifted to the Port E pin numbers
#ifndef OPL3_CS_MASKS
#define OPL3_CS_MASKS {1<<5} //{1<<4,1<<5}
#endif
