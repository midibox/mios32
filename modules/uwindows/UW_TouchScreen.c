/*!
 * \file 	[UW_TouchScreen.c]
 *
 * \author 	[Bassem]
 * \date	06-21-09
 *
 * To Handle Touch Screen Operations.
 */
#include "circle_api.h"
#include "UWindows.h"
#include "Calibrate.h"

POINT 		screensample[3];
POINT 		displaysample[3];
MATRIX		matrix;u8 			calibration_phase;
u8 			lasttouchstatus;

/*!
 * \brief Return if Touchs screen is clicked
 *
 * This function will detect a toggle in TS status and return True when TS is pressed
 * after being released before
 * 
 * \return none 
 */
bool UW_TSIsClicked(void)
{
	/**/
	u8 newtouchstatus = TOUCHSCR_IsPressed();
	if ( (lasttouchstatus == 0) && (newtouchstatus == 1))
	{
		lasttouchstatus = newtouchstatus;
		return 1;
	}
	else
	{
		lasttouchstatus = newtouchstatus;
		return 0;
	}
	
	//return (TOUCHSCR_IsPressed());
}

/*!
 * \brief Return if Touchs screen is clicked
 *
 * This function will draw a cursor for calibration
 * 
 * \return none 
 */
void UW_TSDrawCurser(u8 x, u8 y)
{
	LCD_DrawRect(x-5, y, 10, 1, BLUE);
	LCD_DrawRect(x,y-5,1,10,BLUE);
}

/*!
 * \brief Perform a 3 point Calibration
 *
 * The Calibration algorithm is done on 3 points this function implements it.
 * 
 * \return none 
 */
u8	UW_TSCalibrate( void )
{
	#ifdef	CALIBRATION
	if (calibration_phase == 2)
      {
      		//Third point of calibration
      		UW_TSDrawCurser(85, 50);
		if (UW_TSIsClicked() == 1)
		{
			u16 u16pos;			
			displaysample[calibration_phase].x = 85;
			displaysample[calibration_phase].y = 50;
			u16pos = TOUCHSCR_GetAbsPos();
			screensample[calibration_phase].x= u16pos & 0xFF;
			screensample[calibration_phase].y= (u16pos >> 8) & 0xFF;
			calibration_phase ++;
			setCalibrationMatrix(&displaysample, &screensample, &matrix);
			DRAW_Clear();
		}
	}
	if (calibration_phase == 1)
      {
      		//Second point of calibration
      		UW_TSDrawCurser(110, 110);
		if (UW_TSIsClicked() == 1)
		{
			u16 u16pos;
			displaysample[calibration_phase].x = 110;
			displaysample[calibration_phase].y = 110;
			u16pos = TOUCHSCR_GetAbsPos();
            		screensample[calibration_phase].x= u16pos & 0xFF;
            		screensample[calibration_phase].y= (u16pos >> 8) & 0xFF;
			calibration_phase ++;
		}
	}
	if (calibration_phase == 0)
	{
      		//first point of calibration
      		UW_TSDrawCurser(15, 15);
		if (UW_TSIsClicked() == 1)
		{
			u16 u16pos;
			displaysample[calibration_phase].x = 15;
			displaysample[calibration_phase].y = 15;
			u16pos = TOUCHSCR_GetAbsPos();
            		screensample[calibration_phase].x= u16pos & 0xFF;
            		screensample[calibration_phase].y= (u16pos >> 8) & 0xFF;
         		calibration_phase ++;
		}
	}
	if (calibration_phase <3)
		return MENU_CONTINUE; 
	else
	#endif
		return 0;
		
}
/*!
 * \brief retur a clibratin pointer
 *
 * The function will retun a clibrated position (if Calibration enabled)
 * 
 * \return none 
 */
u16	UW_TSGetPos(void)
{

		POINT	display, screen;
	    u16 u16pos;
		u16pos = TOUCHSCR_GetPos();
		#ifdef CALIBRATION
		screen.x= (u8) ((u16pos & 0xFF) );
		screen.y= (u8)  (((u16pos >> 8) & 0xFF));
		getDisplayPoint(&display,&screen,&matrix);
		u16pos	= (display.x & 0xff) + ((display.y & 0xFF)<<8);
		#endif
		return u16pos;
}
/*!
 * \brief Initialize TS variables
 *
 * 
 * \return none 
 */
u16 UW_TSInit(void)
{
	calibration_phase =0;
    lasttouchstatus = 0;
	//TOUCHSCR_SetSensibility(3000); 
}
