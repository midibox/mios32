/******************** (C) COPYRIGHT 2006 STMicroelectronics ********************
* File Name          : 91x_rtc.h
* Author             : MCD Application Team
* Date First Issued  : 05/18/2006 : Version 1.0
* Description        : This file provides the RTC library software functions
*                      prototypes & definitions
********************************************************************************
* History:
* 05/24/2006 : Version 1.1
* 05/18/2006 : Version 1.0
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH
* CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS
* A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT
* OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
* OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
* CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __91x_RTC_H
#define __91x_RTC_H

/* Includes ------------------------------------------------------------------*/
#include "91x_map.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  u8 century;
  u8 year;
  u8 month;
  u8 day;
  u8 weekday;
}RTC_DATE;

typedef struct
{
  u8 hours;
  u8 minutes;
  u8 seconds;
  u16 milliseconds;
}RTC_TIME;
	
typedef struct
{
  u8 day;
  u8 hours;
  u8 minutes;
  u8 seconds;
}RTC_ALARM;

/* Exported constants --------------------------------------------------------*/

#define BINARY 0
#define BCD 1

/*TamperMode*/
#define RTC_TamperMode_Edge  0xFFFFFFEF
#define RTC_TamperMode_Level 0x10

/*TamperPol*/
#define RTC_TamperPol_High  0x4
#define RTC_TamperPol_Low   0xFFFFFFFB

/*PeriodicClock*/
#define RTC_PER_2Hz      	0x00010000
#define RTC_PER_16Hz     	0x00020000
#define RTC_PER_128Hz    	0x00040000
#define RTC_PER_1024Hz   	0x00080000
#define RTC_PER_DISABLE  	0x0

/*RTC_IT*/
#define RTC_ALARM_EN			0x00100000
#define RTC_IT_PER      	0x00200000
#define RTC_IT_ALARM    	0x00800000
#define RTC_IT_TAMPER   	0x00400000

/*RTC_FLAG*/
#define RTC_FLAG_PER     	0x80000000
#define RTC_FLAG_ALARM   	0x40000000
#define RTC_FLAG_TAMPER  	0x10000000


#define RTC_WRTIE_ENABLE 	0x00000080
#define RTC_CC_OUT_ENABLE 	0x00000040
#define RTC_SRAM_VBATT		0x00000008


#endif /*__91x_RTC_H*/

/******************* (C) COPYRIGHT 2006 STMicroelectronics *****END OF FILE****/



