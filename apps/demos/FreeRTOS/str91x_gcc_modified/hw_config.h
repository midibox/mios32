#ifndef HWCONFIG_H_
#define HWCONFIG_H_

#include "91x_type.h"
#include "91x_map.h"
#include "91x_fmi.h"
#include "91x_vic.h"
#include "91x_scu.h"
#include "91x_emi.h"
#include "91x_rtc.h"
#include "91x_dma.h"

#include "typedefs.h"
#include "fnrtime.h"

#define DAC				I2C0
#define DAC_ADDRESS		0x34

#define COMPASS			I2C1
#define COMPASS_WRITE	0x42
#define COMPASS_READ	0x43


#define SDCARD			SSP0
#define DAC_SPI			SSP1



#define HW_SDCard_CS_High (GPIO5->DR[PORT_BIT7] = 0x80)
#define HW_SDCard_CS_Low  (GPIO5->DR[PORT_BIT7] = 0)


void InitInterruptVecotrs(void);

void InitGPIOPorts(void);
void InitEMIBus(void);

void GoFast(void);


void InitSSP(u8 speed); /* speed=0 is 100khz clock for init, speed=1 is 4Mhz clock */
void InitDMA(void);
void InitI2C(void);
void InitAudio(void);
void InitDAC(void);

void RTC_Init(void);
void RTC_EnableAlarm(u8 en);
void RTC_SetClockTime(fnr_time_t t);
fnr_time_t RTC_GetClockTime(struct tm* ts);
u8 ByteToBCD(u8 value);
u8 BCDtoByte(u8 value);

#endif /*HWCONFIG_H_*/
