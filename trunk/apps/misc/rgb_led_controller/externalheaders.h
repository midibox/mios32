//fixes

#define PCB_BUG_SRIO_EXCHANGED

#define DISPLAY_LED_NUMBER				64		//the maximum number of LEDs per core

//unsigned char is compatible with both projects
extern const unsigned char ab_sysex_header[2];
extern const unsigned char ab_sysex_footer;

//structure of the first three data bytes
typedef struct { //aligned to endianness
	unsigned commandType:5; //the type of message (i.e. the command) (0-31)
	unsigned tripleClick:1; //triple click for preset reset
	unsigned longClick:1; //long click (click & hold)
	unsigned doubleClick:1;
	unsigned device:6 ; //device id (0-63)
	unsigned allControls:1 ; //whether this is for all controls (0 or 1)
	unsigned allDevices:1 ; //whether this is for all devices (0 or 1)
	unsigned char controlNumber; //control number (0-255)
} ab_command_header_t;

//message header length

#define AB_CMD_HEADER_LENGTH 3
#define AB_CMD_RECEIVECONTROL_DATA_SIZE 15

//some commands go directly to other specific classes. since this is the base class, the definition ist stored here
//																																|Data Bytes		
//command values												Description								|0			|1			|2		|3		|4		|5		|6		|7		|7	8	9	
//system commands
#define AB_CMD_ACK								0x1f	//Acknowledge							|0xff		|0xff		|
#define AB_CMD_DISACK							0x1e	//Disacknowledge					|0xff		|0xff		|
#define AB_CMD_RESET							0x1d	//Reset the cores					|0xff		|0xff		|
#define AB_CMD_PREFERENCES				0x14	//Set Device Preferences  |Device	|0xff		|Brigthness(1 Byte)|doubleclickInterval(2Bytes)|holdClickInterval(2Bytes)
#define	AB_CMD_SENDCONFIG					0x1c	//Send config data				|Device	|

//control commands
#define AB_CMD_RECEIVECONTROL			0x1b  //Receive control data		|Device	|Control|Data  THIS CAN BE SENT MULTIPLE TIMES, resending Control and Data. Data is 15 Bytes long.
#define AB_CMD_FLASH							0x1a	//Flash										|Device |control|Red	|Green	| Blue	
#define AB_CMD_BLINK							0x12	//Set Blink state					|Device |control|ON/OFF|	
#define AB_CMD_VALUE							0x13	//Sends a value to a device
#define	AB_CMD_SHORTFLASH					0x11  //Flash very briefly
#define	AB_CMD_SETCOLOR						0x10  //Set Control Color

//clip commands
#define AB_CMD_CLIP_RUN						0x19  //Clip is running
#define AB_CMD_CLIP_SET						0x18  //Clip is set to run
#define AB_CMD_CLIP_RERUN					0x17  //Clip is running and set to re-run
#define AB_CMD_CLIP_STOP					0x16	//Device should stop clip
#define AB_CMD_CLIP_START					0x15	//Device should indicate that clip is starting from beginning

//control types
//these values have to the same as the numbers on the control application in CTConfigTypes.plist
#define AB_CMD_CLIP								0x02	//Clip single click from device
#define AB_CMD_SETCHANGE					0x03	//Set Change							|Device	|control|
#define AB_CMD_SET_VALUE					0x01	//controller has been updated manually
#define AB_CMD_PRESET							0x04	//preset button
#define AB_CMD_MUTE								0x05	
#define AB_CMD_SOLO								0x06	

//command flags
#define AB_CMD_FLAG_DOUBLE				0x80 //double click flag (7th bit of the type byte)
#define AB_CMD_FLAG_TRIPLE				0x20 //triple click flag (5th bit of the type byte)
#define AB_CMD_FLAG_LONG					0x40 //long click flag (6th bit of the type byte)

//control values

#define	CONTROL_BUTTON_ON			127
#define	CONTROL_BUTTON_OFF		0

#define DISPLAY_BLINK_OFF				0
#define DISPLAY_BLINK_ON					1

//these values have to the same as the numbers on the control application in CTConfigTypes.plist
typedef enum {
	controllerTypeNothing = 0, 		//there is no hardware
	controllerTypeSlave = 1,				//these are the second buttons of encoder or forcesensor. always the one with the greater number
	controllerTypeSmallButton = 2,	//sofware can reassign from momentary to on/off
	controllerTypeBigButton = 3,			//this is fixed in the hardware
	controllerTypeEncoder = 4		//this is fixed in the hardware
} controllerType;

//these values have to the same as the numbers on the control application in CTConfigTypes.plist
typedef enum {
	controlTypeNothing = 0,
	controlTypeValue = AB_CMD_SET_VALUE,
	controlTypeSetchange = AB_CMD_SETCHANGE,
	controlTypeClip = AB_CMD_CLIP,
	controlTypePreset = AB_CMD_PRESET,
	controlTypeMute = AB_CMD_MUTE,
	controlTypeSolo = AB_CMD_SOLO
} controlType;

/*
#define	KNOEPFLI_CMD_INIT								0x03	//Init the drivers				|0xff		|0xff	|
//control commands
#define KNOEPFLI_CMD_SETCONTROL					0x04	//Set a control						|L
//direct commands
#define	KNOEPFLI_CMD_SETCOLOR						0x03	//Set one LED to a color	|Address			|Red	|Green	|Blue	|
#define	KNOEPFLI_CMD_SETCOLORALL				0x04	//Set all LEDs to a color	|0xff		|0xff	|Red	|Green	|Blue	|
#define	KNOEPFLI_CMD_SETBLINK						0x05	//Set one LED to blink		|Address			|Red	|Green	|Blue	|
#define	KNOEPFLI_CMD_SETBLINKALL				0x06	//Set all LEDs to blink		|0xff		|0xff	|Red	|Green	|Blue	|
#define	KNOEPFLI_CMD_BLINK							0x07	//Blink one LED						|Address			|
#define	KNOEPFLI_CMD_BLINKALL						0x08	//Blink all LEDs					|0xff		|0xff	|
#define	KNOEPFLI_CMD_SETFLASH						0x09	//Set Flash for one LED		|Address			|Red	|Green	|Blue	|
#define	KNOEPFLI_CMD_SETFLASHALL				0x0a	//Set Flash for all LEDs	|0xff		|0xff	|Red	|Green	|Blue	|
#define	KNOEPFLI_CMD_FLASH							0x0b	//Flash an LED						|Address			|
#define KNOEPFLI_CMD_FLASHALL						0x0c	//Flash all LEDs					|0xff		|0xff	|
#define	KNOEPFLI_CMD_SETBRIGHTNESSALL		0x0d	//Set the overall brightn.|0xff		|0xff	|Brightn|*/

