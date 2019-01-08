#ifndef _DISPLAY_H
#define _DISPLAY_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define DISPLAY_BLINK_STEPS			12		//the number of steps for a blink cycle
#define DISPLAY_FLASH_STEPS			30		//the number of steps for a flash cycle
#define DISPLAY_TOUCH_STEPS			20		//the number of steps for a touch cycle
#define DISPLAY_CHANGE_STEPS	 	 	 3		//the number of steps change between statuses

#define DISPLAY_FLASH_OFF				0
#define DISPLAY_FLASH_ON					1

#define DISPLAY_TOUCH_OFF				0
#define DISPLAY_TOUCH_ON					1

/////////////////////////////////////////////////////////////////////////////
// Global types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
	u8 Driver;    //seven bits, the highest one is 0 if the address is inactive!
	u8 Led;       //we use the number already + 0x02, to avoid conversion into the address. So we start at 0x02 and end at 0x11	   
} display_color_address_t;

typedef struct {
	display_color_address_t Red;
	display_color_address_t Green;
	display_color_address_t Blue;
} display_led_address_t;

//a 24bit RGB color
typedef struct {
	u8 Red;
	u8 Green;
	u8 Blue;
} display_led_color_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 DISPLAY_Init(u8 mode);
extern void DISPLAY_SendColor(u8 number, display_led_color_t color);
extern void DISPLAY_Timer(void);
extern void DISPLAY_SetBlink(u8 number, u8 state);
extern void DISPLAY_Flash(u8 number, display_led_color_t *color);
extern void DISPLAY_MeterFlash(u8 number, display_led_color_t *color);
extern void DISPLAY_Touch(u8 number);
extern void DISPLAY_SendConfig(void);
extern void DISPLAY_UpdateColor(u8 number);
extern void DISPLAY_SetBrightness(u8 brightness);
extern void DISPLAY_LedCheck(); 
extern void DISPLAY_SetColor(u8 number, display_led_color_t *color);
extern void DISPLAY_LedHandler();
extern void DISPLAY_SuspendLedTask();
extern void DISPLAY_ResumeLedTask();
	
/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _DISPLAY_H */
