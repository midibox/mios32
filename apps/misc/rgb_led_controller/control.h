#include "externalheaders.h"
#include "display.h"

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define CONTROL_DOUBLECLICK_TIME 3000
#define CONTROL_LONGCLICK_TIME 4000

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

//this struct cannot be used directly in control_t, it messes up the storage
//and we cannot write into the struct directly anymore. But we leave it here
//for documentation
typedef struct {
	unsigned useTouch:1;  //use touch color for flashing small button when touched
	unsigned inPreset:1;  //Value has been set manually
	unsigned clipSet:1; //if we are a clip button: are we clicked to be run or re-run?
 	unsigned :1; 
	unsigned :1; 
	unsigned :1;
	unsigned :1;
	unsigned :1;
} settings_t;

#define CONTROL_USETOUCH_FLAG 0x80
#define CONTROL_INPRESET_FLAG 0x40
#define CONTROL_CLIPSET_FLAG 0x20

typedef union {
	
	struct {
		u8 Type;
		
		u8 Value; //the current value of the control.
		
		u8 Settings; //some Flags, see above
		
		display_led_color_t mainColor; 
		display_led_color_t secondaryColor; 
		display_led_color_t blinkColor; 
		display_led_color_t touchColor; //until here the values can be overwritten by sysex.c
		
		u8 DoNotUse; //this is overwritten when writing multiple controls at once, content is not defined!
		
		display_led_color_t tempColor;
		display_led_color_t flashColor; 		
		display_led_color_t maxValueColor; //calculated from the main color 		
		display_led_color_t minValueColor; //calculated from the main color
		display_led_color_t ColorNow; //contains the current color of the LED, the one that can be seen at that moment, except for flashes and change transitions
		display_led_color_t ColorFrom; //has to be last, is not set by incoming sysex data (saves us three bytes)
		
		u32 clickTime; //saved time of the last button down
		u8 doubleClick;
		u8 tripleClick;
		
		u8 setTempColor; //flag whether to set the tempcolor
		
		u8	Blink; //blink active
		u8	BlinkSteps; //duration of blinking period

		u8	Flash; //button flashing at the moment
		u8	FlashSteps; //how long the button should flash
		u8	FlashCounter; //where is the button at the moment in the flash cycle?
		
		u8	Touch; //button flashing because of touch at the moment
		u8	TouchSteps; //how long the button should flash
		u8	TouchCounter; //where is the button at the moment in the flash cycle?

		u8	ChangeSteps; //Steps of change for blink/noblink changed or color changed in blink or noblink. Also serves as updated flag
	};
	
	u8 bytes[50];
} control_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void CONTROL_Init();
extern void CONTROL_UpdateControl(u8 number);
extern void CONTROL_ButtonHandler(u32 pin, u32 pin_value);
extern void CONTROL_EncoderHandler(u32 encoder, s32 incrementer);
extern void CONTROL_ClipHandler(u8 number, u8 clipState);
extern void CONTROL_setIntervals(u8* ptr);
extern void CONTROL_SetValue(u8 number, u8 value);
extern void CONTROL_SetInputState(u8 state);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern control_t controls[DISPLAY_LED_NUMBER]; //the data for all controls
extern const u8 control_rows;
extern const u8 control_columns;
extern const u8 controllers[DISPLAY_LED_NUMBER]; 

