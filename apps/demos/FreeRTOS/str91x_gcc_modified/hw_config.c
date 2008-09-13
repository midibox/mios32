#include <string.h>
#include "hw_config.h"
#include "typedefs.h"
#include "fnrtime.h"
#include "vectors.h"

#define VIC_REGISTER_NUMBER	16
#define VIC_PROTECTION_ENABLE_MASK       0x1
#define VIC_PROTECTION_DISABLE_MASK      0xFFFFFFFE
#define VIC_VECTOR_ENABLE_MASK           0x20
#define VIC_IT_SOURCE_MASK               0xFFFFFFE0

void SetupInterrupt(u16 VIC_Source,u8 VIC_Priority, FunctionalState VIC_NewState,VIC_ITLineMode VIC_LineMode, void (*VIC_VectAddress)(void)  );



void Delay(u32 p)
{
	u32 d;
	while(p>0) {
		for(d=0;d<10000;d++) {
			asm volatile ("nop");
		}		
	}
	
}

void SetupInterrupt(u16 VIC_Source,u8 VIC_Priority, FunctionalState VIC_NewState,VIC_ITLineMode VIC_LineMode, void (*VIC_VectAddress)(void)  )
{
	VIC_TypeDef* vic;
	
	if(VIC_Source < 16 ) {
		vic = VIC0;
	} else {
		vic=VIC1;
		VIC_Source -= 16;
	}
	
	vic->VAiR[VIC_Priority] = (u32)VIC_VectAddress;
	vic->VCiR[VIC_Priority] |= VIC_VECTOR_ENABLE_MASK;
    vic->VCiR[VIC_Priority] &= VIC_IT_SOURCE_MASK;
    vic->VCiR[VIC_Priority] |= VIC_Source;
    
    if (VIC_NewState == ENABLE) {
        vic->INTER |= (1 << VIC_Source);
    } else {
        vic->INTECR |= (1 << VIC_Source);
    }    
    
    if (VIC_LineMode == VIC_IRQ) {
    	vic->INTSR &= ~(1 << VIC_Source);
	} else {
		vic->INTSR |= (1 << VIC_Source);
	}
    
}

void InitInterruptVecotrs(void)
{
	u32 l;
	
	SCU->PCGR0 |= __VIC;

	SCU->PRR0 &= ~__VIC;
	SCU->PRR0 |= __VIC;
	
	for(l=0;l<32;l++) {
		SetupInterrupt(l,l,DISABLE,VIC_IRQ,0);
	}
	
	/*
	VIC_Config(SSP1_ITLine,VIC_IRQ , 6);
	VIC_ITCmd(SSP1_ITLine, ENABLE);
	
	
	VIC_Config(DMA_ITLine,VIC_IRQ , 6);
	VIC_ITCmd(DMA_ITLine, ENABLE);
	*/
}


void InitGPIOPorts(void)
{
	

	SCU->PCGR1 |= (__GPIO9 | __GPIO8 | __GPIO7 | __GPIO6 | __GPIO5 | __GPIO4 | __GPIO3 | __GPIO2 | __GPIO1 | __GPIO0  );
	SCU->PRR1 |= (__GPIO9 | __GPIO8 | __GPIO7 | __GPIO6 | __GPIO5 | __GPIO4 | __GPIO3 | __GPIO2 | __GPIO1 | __GPIO0  );

#if 0	
	SCU->GPIOOUT[0] = ( B2_ALTO2 | B3_ALTO2 | B5_ALTO1 | B6_ALTO1 | B7_ALTO1 );
	SCU->GPIOIN[0]  = 0x0c; 
	SCU->GPIOTYPE[0] = ( BIT_2 | BIT_3 );
	 
	GPIO0->DDR = BIT_7 | BIT_6 | BIT_5;

	SCU->GPIOOUT[1] = ( B0_ALTO3 | B1_ALTO3 | B2_ALTO1 );  /* B0_ALTO3 | B1_ALTO3 | for master ssp1*/
	GPIO1->DDR = BIT_2;
	GPIO1->DR[PORT_BIT2]=BIT_2; /* osc enable */

	SCU->GPIOOUT[2] = ( B0_ALTO2 | B1_ALTO2 | B2_ALTO1 );
	SCU->GPIOIN[2] |= ( B0_ALTI | B1_ALTI );
	SCU->GPIOTYPE[2] |= ( BIT_0 | BIT_1 );
	 
	GPIO2->DDR  = BIT_2;
	

	SCU->GPIOOUT[3] = ( B0_ALTO1);
	GPIO3->DDR = BIT_0 ;


	SCU->GPIOOUT[4] =  B6_ALTO2;

	SCU->GPIOOUT[5] = ( B4_ALTO2 | B5_ALTO2 | B7_ALTO1);
	SCU->GPIOIN[5] = ( B6_ALTI );
	
	GPIO5->DDR = 0x80; /* gpio5.7 output */

	SCU->GPIOOUT[6] = 0;

	SCU->GPIOOUT[7] = ( B0_ALTO2 | B5_ALTO3 );

	SCU->GPIOIN[7] = B6_ALTI;
#endif

	SCU->GPIOOUT[9] = BIT_0;
	GPIO9->DDR = BIT_0;
	
}

void InitEMIBus(void)
{
	/* assumes GPIO pins have already been setup */

	SCU->SCR0 |= 0x0040;	/* non mux mode */

	EMI_Bank2->ICR = 10;	/* idle cycles orig 9*/
	
	EMI_Bank2->RCR = 7;	/* read wait states */
	EMI_Bank2->WCR = 1;	/* write wait states orig 0*/
	EMI_Bank2->OECR = 1;	
	EMI_Bank2->WECR = 1;		/* orig 0 */
	EMI_Bank2->BCR = 0;

	SCU->PCGR0 |= __EMI | __EMI_MEM_CLK; 
	SCU->PRR0 |= __EMI;
	SCU->GPIOEMI = 1;
	

}

void GoFast(void)
{
	/* setup clocks assuming 12Mhz main osc
	 * 
	 * 	_PLL_M	6
	 * 	_PLL_N	96
	 * 	_PLL_P	2
	 *
	 * 	R_Clk div1  	96Mhz
	 *    H_Clk div1		48Mhz
	 * 	P_Clk	div2		48Mhz
	 * 	EMI clk div2	24Mhz
	 * 	FMI_Clk div1	96Mhz
	 */

#define _PLL_M	(u32)6
#define _PLL_N	(u32)96
#define _PLL_P	(u32)2

	SCU->CLKCNTR = 0x00020002;	/* make sure not running from pll */
		
	SCU->PLLCONF &= ~0x0080000;	/* disable pll */	

	SCU->SYSSTATUS = 3;	/* clear pll lock bits */

	SCU->PLLCONF = ( (_PLL_P<<16 ) | (_PLL_N << 8 ) | _PLL_M );
	SCU->PLLCONF |= 0x0080000;		/* enable pll */
	
	SCU->CLKCNTR = 0x000200a2;
	
	/* wait to lock */
	while( (SCU->SYSSTATUS &1) ==0 ) {
		asm volatile ("nop");
	}
	
	SCU->CLKCNTR = 0x000200a0; /* swtich to pll clock */
	
}


void InitSSP(u8 speed)
{
	SCU->PCGR1 |= __SSP0;
	SCU->PRR1 |= __SSP0;
	
	if(speed ==2) {
		SDCARD->CR0 = 0x00c7;
		SDCARD->CR1 = 0x0002;
		SDCARD->PR = 2;		/*  24Mhz clk */
	} else if(speed ==1) {
		SDCARD->CR0 = 0x00c7;
		SDCARD->CR1 = 0x0002;
		SDCARD->PR = 2;		/*  12Mhz clk */
	} else {
		SDCARD->CR0 = 0x09c7;
		SDCARD->CR1 = 0x0002;
		SDCARD->PR = 24;		/*  100Khz clk */
	}
	
}

void InitAudio(void)
{

	/* setup timer */

	SCU->PCGR1 |= __TIM23;
	SCU->PRR1 |= __TIM23;
	
	SCU->CLKCNTR |= 0x00004000;


	TIM3->CR1 = 0x0000;
	TIM3->CR2 = 0x0000;
	TIM3->CR1 = 0x0253;
	TIM3->OC1R = (16 - 5);
	TIM3->OC2R = (32 - 5);
	TIM3->CR1 |= 0x8000;	

#if 1	 
	SCU->GPIOOUT[1] &= B0_MASK;
	SCU->GPIOOUT[1] |= B0_ALTO1;
	GPIO1->DDR |= 1;
	GPIO1->DR[PORT_BIT0] =0;

	do {
		GPIO1->DR[PORT_BIT0] =1;
		Delay(2);
		GPIO1->DR[PORT_BIT0] =0;
		Delay(2);		
	} while ((TIM3->SR &0x0800)==0);
	
	SCU->GPIOOUT[1] &= B0_MASK;
	SCU->GPIOOUT[1] |= B0_ALTO3;
#endif

	SCU->PCGR1 |= __SSP1;
	SCU->PRR1 |= __SSP1;

	/* setup spi */
	DAC_SPI->CR1 = 0x0000; /* disable ssp */
		
	DAC_SPI->CR0 = 0x00cf;	/* 0x00cf = CPHA=1,CPOL=1 16bit data*/
	DAC_SPI->CR1 = 0x0000; /*  disabled*/

	DAC_SPI->PR = 96; 
	DAC_SPI->IMSCR = 0x08;

	DAC_SPI->DR = 0;
	DAC_SPI->DR = 0;
	DAC_SPI->DR = 0;
	DAC_SPI->DR = 0;
	DAC_SPI->DR = 0;
	DAC_SPI->DR = 0;
	DAC_SPI->DR = 0;
	DAC_SPI->DR = 0;

	/*DAC_SPI->CR1 = 0x0006; *//* slave mode & enable*/
	
	DAC_SPI->CR1 = 0x0002; /* master mode enable */

	InitDAC();	
	
}

void InitDAC(void)
{
	/* set pll for 12.288Mhz output*/

#if 0 
	HWDACWriteRegister(53, 0x31);	
	HWDACWriteRegister(54, 0x26);	
	HWDACWriteRegister(55, 0xe8);	
	HWDACWriteRegister(52, 0x18);	
	
	
	
	HWDACWriteRegister(4, 0x1d);	/* sets 16khz sample rate, use pll */

	HWDACWriteRegister(7,0x00);	/* 16bit right justified slave mode */
	
	HWDACWriteRegister(26, 0x199);	/* turn dac &  l/r spk vol */
	HWDACWriteRegister(8, 0x1cc);	/* sets 16khz sample rate */
	
	HWDACWriteRegister(25, 0x0c0);	
	HWDACWriteRegister(47, 0x0c);	/* turn on mix */
	HWDACWriteRegister(34, 0x100);	/* turn ldac mix */
	HWDACWriteRegister(37, 0x100);	/* turn rdac mix */
	HWDACWriteRegister(49, 0xf7);	/* turn on class d outputs */
	HWDACWriteRegister(5, 0x00);	/* turn off soft mute & de-emphasis */
	HWDACWriteRegister(10, 0xff);	/* ldac vol */
	HWDACWriteRegister(11,0x1ff);	/* rdac vol */
	HWDACWriteRegister(40, 0x70);	/* left volume */
	HWDACWriteRegister(41, 0x170);	/* right volume */

	/*HWDACWriteRegister(7,0x40);		*//* 16bit right justified master mode */
#endif 
}

void InitDMA(void)
{
	SCU->PCGR0 |= __DMA;
	SCU->PRR0 |= __DMA;
	DMA->TCICR = 0xff;
	DMA->EICR = 0xff;
	DMA-> CNFR |= 0x01 ;
}

void InitI2C(void)
{
	SCU->PCGR1 |= (__I2C0 | __I2C1);
	SCU->PRR1 |= (__I2C0 | __I2C1);
	
	GPIO0->DR[PORT_BIT5] = 0; /* turn on compass power */
	
	
	/* COMAPSS & DAC declared in hw_config.h as I2C1 & I2C0 */	
	/* compass chip */
	COMPASS->CCR = 0x70;	/* standard mode bit under 100khz */	
	COMPASS->ECCR = 0x01;
	
	COMPASS->CR = 0x20; /* enable I2C needs to be done twice*/
	COMPASS->CR = 0x20; /* enable I2C needs to be done twice*/	
	
	/* DAC */
	DAC->CCR = 0x70;	/* fast mode approx 400khz */	
	DAC->ECCR = 0x01;
	
	DAC->CR = 0x20; /* enable I2C needs to be done twice*/
	DAC->CR = 0x20; /* enable I2C needs to be done twice*/	
	
}


void RTC_Init(void)
{
	SCU->PCGR1 |= __RTC;
	SCU->PRR1 |= __RTC;
	
	
	RTC->CR = RTC_PER_1024Hz  | RTC_IT_PER ;
	
	
	RTC_SetClockTime(0);
	 
}

void RTC_EnableAlarm(u8 en)
{
	if(en) {
		RTC->CR |= ( RTC_IT_ALARM | RTC_ALARM_EN );		
	} else {
		RTC->CR &= ~( RTC_IT_ALARM | RTC_ALARM_EN );
	}
	
	
}


void RTC_SetAlarmTime(fnr_time_t t)
{
#if 0	
	struct tm ts;
	u32 bcd_time;
	u32 temp;
	
	fnr_gmtime(&t,&ts);
	ts.tm_year+=1900;
		
	temp = ByteToBCD( (ts.tm_mday) );
	bcd_time = (temp<<24);

	temp = ByteToBCD( (ts.tm_hour) );
	bcd_time |= (temp<<16);

	temp = ByteToBCD( (ts.tm_min) );
	bcd_time |= (temp<<8);
	
	temp = ByteToBCD( (ts.tm_sec) );
	bcd_time |= temp;

	RTC->CR |= RTC_WRTIE_ENABLE;
	RTC->ATR = bcd_time;
	RTC->CR &= ~RTC_WRTIE_ENABLE;
#endif	
}



void RTC_SetClockTime(fnr_time_t t)
{
#if 0	
	struct tm ts;
	u32 bcd_date;
	u32 bcd_time;
	u32 temp;
	
	fnr_gmtime(&t,&ts);
	ts.tm_year+=1900;
	
	temp = ByteToBCD( (ts.tm_year/100) );
	bcd_date = (temp<<24);
	
	temp = ByteToBCD( (ts.tm_year%100) );
	bcd_date |= (temp<<16);
	
	temp = ByteToBCD( (ts.tm_mon+1) );
	bcd_date |= (temp<<8);
	
	temp = ByteToBCD( (ts.tm_wday+1) );
	bcd_date |= temp;
	
	temp = ByteToBCD( (ts.tm_mday) );
	bcd_time = (temp<<24);

	temp = ByteToBCD( (ts.tm_hour) );
	bcd_time |= (temp<<16);

	temp = ByteToBCD( (ts.tm_min) );
	bcd_time |= (temp<<8);
	
	temp = ByteToBCD( (ts.tm_sec) );
	bcd_time |= temp;


	RTC->CR |= RTC_WRTIE_ENABLE;
	RTC->TR = bcd_time;
	RTC->DTR = bcd_date;
	RTC->MILR = 0;
	RTC->CR &= ~RTC_WRTIE_ENABLE;
#endif	
}

fnr_time_t RTC_GetClockTime(struct tm* ts)
{
#if 0	
	struct tm tmp_ts;
	fnr_time_t now;
	u32 bcd_date;
	u32 bcd_time;
	u32 temp;
	
	do {
		bcd_date = RTC->DTR;
		bcd_time = RTC->TR;
	} while (bcd_date != RTC->DTR); 
	

	temp = BCDtoByte( (bcd_date>>24) )*100;
	temp += BCDtoByte( (bcd_date>>16) );
	tmp_ts.tm_year = temp -1900;
	
	tmp_ts.tm_mon = BCDtoByte( (bcd_date>>8) ) -1;
	tmp_ts.tm_wday = BCDtoByte( bcd_date ) -1;
	tmp_ts.tm_mday = BCDtoByte( (bcd_time>>24) );
	tmp_ts.tm_hour= BCDtoByte( (bcd_time>>16) );
	tmp_ts.tm_min = BCDtoByte( (bcd_time>>8) );
	tmp_ts.tm_sec = BCDtoByte( bcd_time );	
	
	if(ts !=NULL) {
		memcpy(ts,&tmp_ts,sizeof(struct tm) );
	}
	now = fnr_mktime(&tmp_ts);	
	
	return now;
#endif
} 

u8 ByteToBCD(u8 value)
{
  u8 bcdhigh = 0;
  while (value >= 10)
  {
    bcdhigh++;
    value -= 10;
  }
  return  (bcdhigh << 4) | value;
}

u8 BCDtoByte(u8 value)
{
  u32 tmp;
  tmp= ((value&0xF0)>>4)*10;
  return (u8)(tmp+ (value&0x0F));	
}
