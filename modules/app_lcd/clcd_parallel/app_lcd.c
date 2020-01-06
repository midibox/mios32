// $Id$
/*
 * Application specific LCD driver for up to 4 HD44870 LCDs,
 * connected via individual, parallel 8-wire data + 3-wire control buses.
 * ==========================================================================
 * Idle levels for LCD pins:
 * RS must be set to 0 (command mode; don't remember why it was necessary),
 * RW must be set to 0 (write; don't remember why it was necessary),
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <glcd_font.h>
#include <string.h>

#include "app_lcd.h"

/////////////////////////////////////////////////////////////////////////////
// Pins
/////////////////////////////////////////////////////////////////////////////

struct hd44780_pins {
    GPIO_TypeDef *rs_port;
    GPIO_TypeDef *rw_port;
    GPIO_TypeDef *e_port;
    GPIO_TypeDef *data_port;
    uint16_t rs_pin_mask;
    uint16_t rw_pin_mask;
    uint16_t e_pin_mask;
    uint16_t data_pins_offset;
};

// 4-LCD Layout, compatible with DIY-MORE board

#ifndef MIOS32_CLCD_PARALLEL_DISPLAYS
#define MIOS32_CLCD_PARALLEL_DISPLAYS 1
#endif

#ifndef MIOS32_CLCD_PARALLEL_LCD0_RS_PORT
#define MIOS32_CLCD_PARALLEL_LCD0_RS_PORT GPIOD
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD0_RS_PIN
#define MIOS32_CLCD_PARALLEL_LCD0_RS_PIN GPIO_Pin_4
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD0_RW_PORT
#define MIOS32_CLCD_PARALLEL_LCD0_RW_PORT GPIOD
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD0_RW_PIN
#define MIOS32_CLCD_PARALLEL_LCD0_RW_PIN GPIO_Pin_7
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD0_E_PORT
#define MIOS32_CLCD_PARALLEL_LCD0_E_PORT GPIOD
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD0_E_PIN
#define MIOS32_CLCD_PARALLEL_LCD0_E_PIN GPIO_Pin_3
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD0_DATA_PORT
#define MIOS32_CLCD_PARALLEL_LCD0_DATA_PORT GPIOD
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD0_DATA_PINS_OFFSET
#define MIOS32_CLCD_PARALLEL_LCD0_DATA_PINS_OFFSET 8U
#endif


#ifndef MIOS32_CLCD_PARALLEL_LCD1_RS_PORT
#define MIOS32_CLCD_PARALLEL_LCD1_RS_PORT GPIOA
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD1_RS_PIN
#define MIOS32_CLCD_PARALLEL_LCD1_RS_PIN GPIO_Pin_13
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD1_RW_PORT
#define MIOS32_CLCD_PARALLEL_LCD1_RW_PORT GPIOA
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD1_RW_PIN
#define MIOS32_CLCD_PARALLEL_LCD1_RW_PIN GPIO_Pin_14
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD1_E_PORT
#define MIOS32_CLCD_PARALLEL_LCD1_E_PORT GPIOA
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD1_E_PIN
#define MIOS32_CLCD_PARALLEL_LCD1_E_PIN GPIO_Pin_15
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD1_DATA_PORT
#define MIOS32_CLCD_PARALLEL_LCD1_DATA_PORT GPIOB
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD1_DATA_PINS_OFFSET
#define MIOS32_CLCD_PARALLEL_LCD1_DATA_PINS_OFFSET 0U
#endif


#ifndef MIOS32_CLCD_PARALLEL_LCD2_RS_PORT
#define MIOS32_CLCD_PARALLEL_LCD2_RS_PORT GPIOC
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD2_RS_PIN
#define MIOS32_CLCD_PARALLEL_LCD2_RS_PIN GPIO_Pin_0
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD2_RW_PORT
#define MIOS32_CLCD_PARALLEL_LCD2_RW_PORT GPIOC
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD2_RW_PIN
#define MIOS32_CLCD_PARALLEL_LCD2_RW_PIN GPIO_Pin_2
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD2_E_PORT
#define MIOS32_CLCD_PARALLEL_LCD2_E_PORT GPIOC
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD2_E_PIN
#define MIOS32_CLCD_PARALLEL_LCD2_E_PIN GPIO_Pin_3
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD2_DATA_PORT
#define MIOS32_CLCD_PARALLEL_LCD2_DATA_PORT GPIOE
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD2_DATA_PINS_OFFSET
#define MIOS32_CLCD_PARALLEL_LCD2_DATA_PINS_OFFSET 0U
#endif


#ifndef MIOS32_CLCD_PARALLEL_LCD3_RS_PORT
#define MIOS32_CLCD_PARALLEL_LCD3_RS_PORT GPIOC
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD3_RS_PIN
#define MIOS32_CLCD_PARALLEL_LCD3_RS_PIN GPIO_Pin_13
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD3_RW_PORT
#define MIOS32_CLCD_PARALLEL_LCD3_RW_PORT GPIOA
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD3_RW_PIN
#define MIOS32_CLCD_PARALLEL_LCD3_RW_PIN GPIO_Pin_3
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD3_E_PORT
#define MIOS32_CLCD_PARALLEL_LCD3_E_PORT GPIOA
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD3_E_PIN
#define MIOS32_CLCD_PARALLEL_LCD3_E_PIN GPIO_Pin_6
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD3_DATA_PORT
#define MIOS32_CLCD_PARALLEL_LCD3_DATA_PORT GPIOE
#endif
#ifndef MIOS32_CLCD_PARALLEL_LCD3_DATA_PINS_OFFSET
#define MIOS32_CLCD_PARALLEL_LCD3_DATA_PINS_OFFSET 8U
#endif


static struct hd44780_pins displays[MIOS32_CLCD_PARALLEL_DISPLAYS] = {
    {
        .rs_port            = MIOS32_CLCD_PARALLEL_LCD0_RS_PORT,
        .rs_pin_mask        = MIOS32_CLCD_PARALLEL_LCD0_RS_PIN,
        .rw_port            = MIOS32_CLCD_PARALLEL_LCD0_RW_PORT,
        .rw_pin_mask        = MIOS32_CLCD_PARALLEL_LCD0_RW_PIN,
        .e_port             = MIOS32_CLCD_PARALLEL_LCD0_E_PORT,
        .e_pin_mask         = MIOS32_CLCD_PARALLEL_LCD0_E_PIN,
        .data_port          = MIOS32_CLCD_PARALLEL_LCD0_DATA_PORT,
        .data_pins_offset   = MIOS32_CLCD_PARALLEL_LCD0_DATA_PINS_OFFSET,
    },
#if MIOS32_CLCD_PARALLEL_DISPLAYS >= 2
    {
        .rs_port            = MIOS32_CLCD_PARALLEL_LCD1_RS_PORT,
        .rs_pin_mask        = MIOS32_CLCD_PARALLEL_LCD1_RS_PIN,
        .rw_port            = MIOS32_CLCD_PARALLEL_LCD1_RW_PORT,
        .rw_pin_mask        = MIOS32_CLCD_PARALLEL_LCD1_RW_PIN,
        .e_port             = MIOS32_CLCD_PARALLEL_LCD1_E_PORT,
        .e_pin_mask         = MIOS32_CLCD_PARALLEL_LCD1_E_PIN,
        .data_port          = MIOS32_CLCD_PARALLEL_LCD1_DATA_PORT,
        .data_pins_offset   = MIOS32_CLCD_PARALLEL_LCD1_DATA_PINS_OFFSET,
    },
#endif
#if MIOS32_CLCD_PARALLEL_DISPLAYS >= 3
    {
        .rs_port            = MIOS32_CLCD_PARALLEL_LCD2_RS_PORT,
        .rs_pin_mask        = MIOS32_CLCD_PARALLEL_LCD2_RS_PIN,
        .rw_port            = MIOS32_CLCD_PARALLEL_LCD2_RW_PORT,
        .rw_pin_mask        = MIOS32_CLCD_PARALLEL_LCD2_RW_PIN,
        .e_port             = MIOS32_CLCD_PARALLEL_LCD2_E_PORT,
        .e_pin_mask         = MIOS32_CLCD_PARALLEL_LCD2_E_PIN,
        .data_port          = MIOS32_CLCD_PARALLEL_LCD2_DATA_PORT,
        .data_pins_offset   = MIOS32_CLCD_PARALLEL_LCD2_DATA_PINS_OFFSET,
    },
#endif
#if MIOS32_CLCD_PARALLEL_DISPLAYS >= 4
    {
        .rs_port            = MIOS32_CLCD_PARALLEL_LCD3_RS_PORT,
        .rs_pin_mask        = MIOS32_CLCD_PARALLEL_LCD3_RS_PIN,
        .rw_port            = MIOS32_CLCD_PARALLEL_LCD3_RW_PORT,
        .rw_pin_mask        = MIOS32_CLCD_PARALLEL_LCD3_RW_PIN,
        .e_port             = MIOS32_CLCD_PARALLEL_LCD3_E_PORT,
        .e_pin_mask         = MIOS32_CLCD_PARALLEL_LCD3_E_PIN,
        .data_port          = MIOS32_CLCD_PARALLEL_LCD3_DATA_PORT,
        .data_pins_offset   = MIOS32_CLCD_PARALLEL_LCD3_DATA_PINS_OFFSET,
    },
#endif
};


/////////////////////////////////////////////////////////////////////////////
// HD44780 stuff
//
// Write operation
//
// RS:
// --\/--------------------------\/-----
// --/\--------------------------/\-----
//
// RW:
// --\                            /-----
// ---\--------------------------/------
//   |  tAS  |
//
//               PWEH
//           +-------------+                +----------
// E:        |             |                |
// ----------+             +----------------+
//
// DATA:         |  tDSW   | tH  |
// -------------\/--------------\/-----
// -------------/\--------------/\-----
//
// tAS      address set-up time min 40nS
// tcycE    enable cycle time   min 500nS
// PWEH     enable pulse width  min 230nS
// tDSW     data set-up time    min 80nS
// tH       data hold time      min 10nS
/////////////////////////////////////////////////////////////////////////////

#define HD44780_LCD__DONT_USE_BUSY_FLAG             1

#define HD44780_LCD__BUSY_TIMEOUT_CYCLES            100
#define HD44780_LCD__ENABLE_PULSE_WIDTH_US          10
#define HD44780_LCD__ADDRESS_SETUP_TIME_US          1
#define HD44780_LCD__EXECUTION_DELAY_US             40
#define HD44780_LCD__SLOW_COMMAND_DELAY_US          1600

#define HD44780_LCD__COMMAND__CLEAR                 0x01U
/**
 * Return home sets DDRAM address 0 into the address counter,
 * and returns the display to its original status, if it was shifted. The DDRAM contents do not change.
 * Max execution time 1.52ms
 */
#define HD44780_LCD__COMMAND__RETURN_HOME           0x02U
#define HD44780_LCD__COMMAND__SCREEN                0x04U
#define HD44780_LCD__COMMAND__SCREEN_ADDRESS_INC    0x02U
#define HD44780_LCD__COMMAND__SCREEN_ADDRESS_DEC    0x00U
#define HD44780_LCD__COMMAND__DISPLAY               0x08U
#define HD44780_LCD__COMMAND__DISPLAY_OFF           0x00U
#define HD44780_LCD__COMMAND__DISPLAY_ON            0x04U
#define HD44780_LCD__COMMAND__DISPLAY_CURSOR_OFF    0x00U
#define HD44780_LCD__COMMAND__DISPLAY_CURSOR_ON     0x02U
#define HD44780_LCD__COMMAND__DISPLAY_CURSOR_BLINK  0x01U
#define HD44780_LCD__COMMAND__CONFIGURE             0x20U
#define HD44780_LCD__COMMAND__CONFIGURE_BUS_4_BIT   0x00U
#define HD44780_LCD__COMMAND__CONFIGURE_BUS_8_BIT   0x10U
#define HD44780_LCD__COMMAND__CONFIGURE_LINES_1     0x00U
#define HD44780_LCD__COMMAND__CONFIGURE_LINES_2     0x08U
#define HD44780_LCD__COMMAND__CONFIGURE_CHAR_5X8    0x00U
#define HD44780_LCD__COMMAND__CONFIGURE_CHAR_5X10   0x04U
#define HD44780_LCD__COMMAND__SET_CGRAM_ADDRESS     0x40U
#define HD44780_LCD__COMMAND__SET_DDRAM_ADDRESS     0x80U

#define HD44780_LCD__LINE0_ADDRESS 0x00U
#define HD44780_LCD__LINE1_ADDRESS 0x40U
#define HD44780_LCD__LINE2_ADDRESS 0x14U
#define HD44780_LCD__LINE3_ADDRESS 0x54U

/////////////////////////////////////////////////////////////////////////////
// Generic hd44780 support
/////////////////////////////////////////////////////////////////////////////

/**
 * Initializes Register Select (RS) pin as output.
 */
void hd44780_lcd__register_select__init(const struct hd44780_pins * const lcd) {
    GPIO_InitTypeDef lcd0__GPIO_InitStructure;
    GPIO_StructInit(&lcd0__GPIO_InitStructure);
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    lcd0__GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    lcd0__GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // weak driver to reduce transients
    lcd0__GPIO_InitStructure.GPIO_Pin = lcd->rs_pin_mask;
    GPIO_Init(lcd->rs_port, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__register_select__command(const struct hd44780_pins * const lcd) {
    MIOS32_SYS_STM_PINSET_0(lcd->rs_port, lcd->rs_pin_mask);
}

void hd44780_lcd__register_select__data(const struct hd44780_pins * const lcd) {
    MIOS32_SYS_STM_PINSET_1(lcd->rs_port, lcd->rs_pin_mask);
}


/**
 * Initializes Read/Write (RW) pin as output.
 */
void hd44780_lcd__mode_init(const struct hd44780_pins * const lcd) {
    GPIO_InitTypeDef lcd0__GPIO_InitStructure;
    GPIO_StructInit(&lcd0__GPIO_InitStructure);
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    lcd0__GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    lcd0__GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // weak driver to reduce transients
    lcd0__GPIO_InitStructure.GPIO_Pin = lcd->rw_pin_mask;
    GPIO_Init(lcd->rw_port, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__mode_read(const struct hd44780_pins * const lcd) {
    MIOS32_SYS_STM_PINSET_1(lcd->rw_port, lcd->rw_pin_mask);
}

void hd44780_lcd__mode_write(const struct hd44780_pins * const lcd) {
    MIOS32_SYS_STM_PINSET_0(lcd->rw_port, lcd->rw_pin_mask);
}


/**
 * Initializes Enable (E) pin as output.
 */
void hd44780_lcd__e__init(const struct hd44780_pins * const lcd) {
    GPIO_InitTypeDef lcd0__GPIO_InitStructure;
    GPIO_StructInit(&lcd0__GPIO_InitStructure);
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    lcd0__GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    lcd0__GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // weak driver to reduce transients
    lcd0__GPIO_InitStructure.GPIO_Pin = lcd->e_pin_mask;
    GPIO_Init(lcd->e_port, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__e_high(const struct hd44780_pins * const lcd) {
    MIOS32_SYS_STM_PINSET_1(lcd->e_port, lcd->e_pin_mask);
}

void hd44780_lcd__e_low(const struct hd44780_pins * const lcd) {
    MIOS32_SYS_STM_PINSET_0(lcd->e_port, lcd->e_pin_mask);
}

void hd44780_lcd__latch_data(const struct hd44780_pins * const lcd) {
    hd44780_lcd__e_high(lcd);
    MIOS32_DELAY_Wait_uS(HD44780_LCD__ENABLE_PULSE_WIDTH_US);
    hd44780_lcd__e_low(lcd);
}


/**
 * Switches data pins to outputs.
 */
void hd44780_lcd__data_port__mode_write(const struct hd44780_pins * const lcd) {
    GPIO_InitTypeDef lcd0__GPIO_InitStructure;
    GPIO_StructInit(&lcd0__GPIO_InitStructure);
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    lcd0__GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    lcd0__GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // weak driver to reduce transients
    lcd0__GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    lcd0__GPIO_InitStructure.GPIO_Pin = 0xFFU << lcd->data_pins_offset;
    GPIO_Init(lcd->data_port, &lcd0__GPIO_InitStructure);
}

/**
 * Switches data pins to inputs, with pull-ups.
 */
void hd44780_lcd__data_port__mode_read(const struct hd44780_pins * const lcd) {
    GPIO_InitTypeDef lcd0__GPIO_InitStructure;
    GPIO_StructInit(&lcd0__GPIO_InitStructure);
    lcd0__GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    lcd0__GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // weak driver to reduce transients
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    lcd0__GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    lcd0__GPIO_InitStructure.GPIO_Pin = 0xFFU << lcd->data_pins_offset;
    GPIO_Init(lcd->data_port, &lcd0__GPIO_InitStructure);
}

/**
 * Switches data pins to inputs, without pull-ups.
 */
void hd44780_lcd__data_port__mode_hi(const struct hd44780_pins * const lcd) {
    GPIO_InitTypeDef lcd0__GPIO_InitStructure;
    GPIO_StructInit(&lcd0__GPIO_InitStructure);
    lcd0__GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    lcd0__GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // weak driver to reduce transients
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    lcd0__GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    lcd0__GPIO_InitStructure.GPIO_Pin = 0xFFU << lcd->data_pins_offset;
    GPIO_Init(lcd->data_port, &lcd0__GPIO_InitStructure);
}


/**
 * Writes a byte of data to the data pins.
 */
void hd44780_lcd__data_port__write(const struct hd44780_pins *const lcd, const uint8_t value) {
    lcd->data_port->ODR = (uint16_t) (value << lcd->data_pins_offset)
        | (lcd->data_port->ODR & ~(0xFFU << lcd->data_pins_offset));
}

uint8_t hd44780_lcd__is_busy(const struct hd44780_pins * const lcd) {
    return MIOS32_SYS_STM_PINGET(lcd->data_port, 1U << (lcd->data_pins_offset + 7U));
}

s32 hd44780_lcd__wait_until_not_busy(const struct hd44780_pins *const lcd, s32 time_out) {
    s32 result = -1;
    hd44780_lcd__data_port__mode_read(lcd);
    hd44780_lcd__mode_read(lcd);
    while (time_out > 0) {
        hd44780_lcd__e_high(lcd);
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");

        if (!hd44780_lcd__is_busy(lcd)) {
            time_out = 0;
            result = 0;
        }

        hd44780_lcd__e_low(lcd);
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");
        __asm__ __volatile__("nop");

        --time_out;
    }
    hd44780_lcd__mode_write(lcd);
    // data pins remain in read mode
    return result;
}

/**
 * Sends a byte to LCD.
 * If Busy flag is used, data pins are normally kept in high-Z mode, so need to switch to Output mode for the write.
 */
void hd44780_lcd__send_byte(const struct hd44780_pins * const lcd, const uint8_t i) {
#ifndef HD44780_LCD__DONT_USE_BUSY_FLAG
    hd44780_lcd__data_port__mode_write(lcd);
#endif
    hd44780_lcd__data_port__write(lcd, i);
    hd44780_lcd__latch_data(lcd);
#ifndef HD44780_LCD__DONT_USE_BUSY_FLAG
    hd44780_lcd__data_port__mode_hi(lcd);
#endif
}

s32 hd44780_lcd__send_command(const struct hd44780_pins * const lcd, const uint8_t i) {
#ifdef HD44780_LCD__DONT_USE_BUSY_FLAG
    MIOS32_DELAY_Wait_uS(HD44780_LCD__EXECUTION_DELAY_US);
#else
    if (hd44780_lcd__wait_until_not_busy(lcd, HD44780_LCD__BUSY_TIMEOUT_CYCLES) < 0) return -1;
#endif
    hd44780_lcd__send_byte(lcd, i);
    return 0;
}

s32 hd44780_lcd__send_data(const struct hd44780_pins * const lcd, const uint8_t i) {
#ifdef HD44780_LCD__DONT_USE_BUSY_FLAG
    MIOS32_DELAY_Wait_uS(HD44780_LCD__EXECUTION_DELAY_US);  // to complete previous operation, if any
#else
    if (hd44780_lcd__wait_until_not_busy(lcd, HD44780_LCD__BUSY_TIMEOUT_CYCLES) < 0) return -1;
#endif

    hd44780_lcd__register_select__data(lcd);
    MIOS32_DELAY_Wait_uS(HD44780_LCD__ADDRESS_SETUP_TIME_US);
    hd44780_lcd__send_byte(lcd, i);
    hd44780_lcd__register_select__command(lcd);
    return 0;
}


void hd44780_lcd__init0(const struct hd44780_pins *const lcd) {
    hd44780_lcd__e__init(lcd);
    hd44780_lcd__e_low(lcd);

    hd44780_lcd__mode_init(lcd);
    hd44780_lcd__mode_write(lcd);

    hd44780_lcd__register_select__init(lcd);
    hd44780_lcd__register_select__command(lcd);

    hd44780_lcd__data_port__mode_write(lcd);
}

void hd44780_lcd__init1(const struct hd44780_pins *const lcd) {
    uint8_t i = 0;
    while (i < 30) {
        hd44780_lcd__data_port__write(lcd, HD44780_LCD__COMMAND__CONFIGURE | HD44780_LCD__COMMAND__CONFIGURE_BUS_8_BIT);
        MIOS32_DELAY_Wait_uS(100);
        hd44780_lcd__latch_data(lcd);
        MIOS32_DELAY_Wait_uS(5000);
        i++;
    }
}

s32 hd44780_lcd__init2(const struct hd44780_pins * const lcd) {
    return hd44780_lcd__send_command(lcd, HD44780_LCD__COMMAND__CONFIGURE | HD44780_LCD__COMMAND__CONFIGURE_BUS_8_BIT | HD44780_LCD__COMMAND__CONFIGURE_LINES_2 | HD44780_LCD__COMMAND__CONFIGURE_CHAR_5X8);
}
s32 hd44780_lcd__init3(const struct hd44780_pins * const lcd) {
    return hd44780_lcd__send_command(lcd, HD44780_LCD__COMMAND__DISPLAY | HD44780_LCD__COMMAND__DISPLAY_ON | HD44780_LCD__COMMAND__DISPLAY_CURSOR_OFF);
}
s32 hd44780_lcd__init4(const struct hd44780_pins * const lcd) {
    return hd44780_lcd__send_command(lcd, HD44780_LCD__COMMAND__SCREEN | HD44780_LCD__COMMAND__SCREEN_ADDRESS_INC);
}
s32 hd44780_lcd__init5(const struct hd44780_pins * const lcd) {
    s32 result = hd44780_lcd__send_command(lcd, HD44780_LCD__COMMAND__RETURN_HOME);
#ifdef HD44780_LCD__DONT_USE_BUSY_FLAG
    MIOS32_DELAY_Wait_uS(HD44780_LCD__SLOW_COMMAND_DELAY_US);
#endif
    return result;
}
s32 hd44780_lcd__init6(const struct hd44780_pins * const lcd) {
    return hd44780_lcd__send_command(lcd, HD44780_LCD__COMMAND__CLEAR);
}


s32 hd44780_lcd__init(const struct hd44780_pins * const lcd) {
    hd44780_lcd__init0(lcd);
    
    MIOS32_DELAY_Wait_uS(4000);

    hd44780_lcd__init1(lcd);

    MIOS32_DELAY_Wait_uS(4000);

    if (hd44780_lcd__init2(lcd) < 0) {
        return -1;
    }
    MIOS32_DELAY_Wait_uS(4000);
    if (hd44780_lcd__init3(lcd) < 0) {
        return -1;
    }
    MIOS32_DELAY_Wait_uS(4000);
    if (hd44780_lcd__init4(lcd) < 0) {
        return -1;
    }
    MIOS32_DELAY_Wait_uS(4000);
    if (hd44780_lcd__init5(lcd) < 0) {
        return -1;
    }

    // according to data sheet, max execution time of RETURN_HOME is 1.52ms

    MIOS32_DELAY_Wait_uS(4000);
    if (hd44780_lcd__init6(lcd) < 0) {
        return -1;
    }
    MIOS32_DELAY_Wait_uS(4000);
    return 0;
    // data pins will remain in high-Z mode
}


void hd44780_lcd__goto(const struct hd44780_pins * const lcd, const uint8_t x, const uint8_t y) {
    uint8_t command;
    switch (y) {
    case 0: command = (HD44780_LCD__COMMAND__SET_DDRAM_ADDRESS | HD44780_LCD__LINE0_ADDRESS) + x;
        break;
    case 1: command = (HD44780_LCD__COMMAND__SET_DDRAM_ADDRESS | HD44780_LCD__LINE1_ADDRESS) + x;
        break;
    case 2: command = (HD44780_LCD__COMMAND__SET_DDRAM_ADDRESS | HD44780_LCD__LINE2_ADDRESS) + x;
        break;
    case 3: command = (HD44780_LCD__COMMAND__SET_DDRAM_ADDRESS | HD44780_LCD__LINE3_ADDRESS) + x;
        break;
    default: command = (HD44780_LCD__COMMAND__SET_DDRAM_ADDRESS | HD44780_LCD__LINE0_ADDRESS) + x;
    }

    hd44780_lcd__send_command(lcd, command);
}


/////////////////////////////////////////////////////////////////////////////
// APP_LCD API implementation
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode) {
    return hd44780_lcd__init(&displays[mios32_lcd_device]);
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data) {
    return hd44780_lcd__send_data(&displays[mios32_lcd_device], data);
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd) {
    return hd44780_lcd__send_command(&displays[mios32_lcd_device], cmd);
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void) {
    s32 result = hd44780_lcd__send_command(&displays[mios32_lcd_device], HD44780_LCD__COMMAND__CLEAR);
#ifdef HD44780_LCD__DONT_USE_BUSY_FLAG
    MIOS32_DELAY_Wait_uS(HD44780_LCD__SLOW_COMMAND_DELAY_US);
#endif
    return result;
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line) {
    hd44780_lcd__goto(&displays[mios32_lcd_device], column, line);
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y) {
    return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8]) {
    s32 i;

    // send command with CGRAM base address for given character
    APP_LCD_Cmd(((num & 7U) << 3U) | HD44780_LCD__COMMAND__SET_CGRAM_ADDRESS);

    // send 8 data bytes
    for (i = 0; i < 8; ++i)
        if (APP_LCD_Data(table[i]) < 0)
            return -1; // error during sending character

    // set cursor to original position
    return APP_LCD_CursorSet(mios32_lcd_column, mios32_lcd_line);
}


/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u32 rgb) {
    return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u32 rgb) {
    return -3; // not supported
}



/////////////////////////////////////////////////////////////////////////////
// Sets a pixel in the bitmap
// IN: bitmap, x/y position and colour value (value range depends on APP_LCD_COLOUR_DEPTH)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour) {
    if (x >= bitmap.width || y >= bitmap.height)
        return -1; // pixel is outside bitmap

    // all GLCDs support the same bitmap scrambling
    u8 *pixel = (u8 *) &bitmap.memory[bitmap.line_offset * (y / 8) + x];
    u8 mask = 1 << (y % 8);

    *pixel &= ~mask;
    if (colour)
        *pixel |= mask;

    return -3; // not supported
}


/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap within given boundaries to the LCD
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap) {
//  if( lcd_testmode )
//    return -1; // direct access disabled in testmode

    if (!MIOS32_LCD_TypeIsGLCD())
        return -1; // no GLCD

    // abort if max. width reached
    if (mios32_lcd_x >= mios32_lcd_parameters.width)
        return -2;

    // all GLCDs support the same bitmap scrambling
    int line;
    int y_lines = (bitmap.height >> 3);

    u16 initial_x = mios32_lcd_x;
    u16 initial_y = mios32_lcd_y;
    for (line = 0; line < y_lines; ++line) {

        // calculate pointer to bitmap line
        u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset;

        // set graphical cursor after second line has reached
        if (line > 0) {
            mios32_lcd_x = initial_x;
            mios32_lcd_y += 8;
            APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
        }

        // transfer character
        int x;
        for (x = 0; x < bitmap.width; ++x)
            APP_LCD_Data(*memory_ptr++);
    }

    // fix graphical cursor if more than one line has been print
    if (y_lines >= 1) {
        mios32_lcd_y = initial_y;
        APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Optional alternative pinning options
// E.g. for MIDIbox CV which accesses a CLCD at J15, and SSD1306 displays at J5/J28 (STM32F4: J5/J10B)
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_AltPinningSet(u8 alt_pinning) {
    return 0; // no error
}

u8 APP_LCD_AltPinningGet(void) {
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Terminal Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_TerminalHelp(void *_output_function) {
    void (*out)(char *format, ...) = _output_function;
    out("Unsupported\n");
    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
    if( word == NULL )
        return -1;

    char *next;
    long l = strtol(word, &next, 0);

    if( word == next )
        return -1;

    return l; // value is valid
}


static void
test_mode__set_lcd_pin_and_report(struct hd44780_pins *lcd, int pin_number, int level, void *_output_function) {
    void (*out)(char *format, ...) = _output_function;

    switch (pin_number) {     
    case 1: {
        if (level) {
            hd44780_lcd__register_select__data(lcd);
        } else {
            hd44780_lcd__register_select__command(lcd);
        }
        out("RS set to %s", level ? "3.3V" : "0V");
        return;
    }
    case 2: {
        if (level) {
            hd44780_lcd__e_high(lcd);
        } else {
            hd44780_lcd__e_low(lcd);
        }
        out("E set to %s", level ? "3.3V" : "0V");
        return;
    }
    case 3: {
        if (level) {
            hd44780_lcd__data_port__mode_write(lcd);
        } else {
            hd44780_lcd__data_port__mode_read(lcd);
        }
        out("DATA DIR is set to %s", level ? "output" : "input");
        return;
    }
    case 4: {
        if (level) {
            hd44780_lcd__mode_read(lcd);
        } else {
            hd44780_lcd__mode_write(lcd);
        }
        out("RW set %s", level ? "3.3V" : "0V");
        return;
    }
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12: {
        u8 d_pin = pin_number - 5;
        hd44780_lcd__data_port__write(lcd, 1U << d_pin);
        out("D%d set to %s", d_pin, level ? "5V (resp. 3.3V)" : "0V");
        return;
    }
    case 14: {
        // d
        hd44780_lcd__data_port__write(lcd, level);
        out("Written %d to data port", level);
        return;
    }
    case 15: {
        // init
        out("Going to init %d", level);
        s32 init_result = 0;
        switch (level) {
        case 0: hd44780_lcd__init0(lcd); init_result = 0; break;
        case 1: hd44780_lcd__init1(lcd); init_result = 0; break;
        case 2: init_result = hd44780_lcd__init2(lcd); break;
        case 3: init_result = hd44780_lcd__init3(lcd); break;
        case 4: init_result = hd44780_lcd__init4(lcd); break;
        case 5: init_result = hd44780_lcd__init5(lcd); break;
        case 6: init_result = hd44780_lcd__init6(lcd); break;
        }
        out("Invoked init %d, returned %d", level, init_result);
        return;
    }
    case 16: {
        // char
        out("Sending %d", level);
        s32 data_result = APP_LCD_Data(level);
        out("Sent data, returned %d", data_result);
        return;
    }
    case 17: {
        // in
        out("Reading port");
        hd44780_lcd__data_port__mode_read(lcd);
        hd44780_lcd__mode_read(lcd);

        hd44780_lcd__e_high(lcd);
        MIOS32_DELAY_Wait_uS(10);
        uint32_t value = lcd->data_port->IDR;
        hd44780_lcd__e_low(lcd);

        hd44780_lcd__mode_write(lcd);
//        hd44780_lcd__data_port__mode_write();
        out("Read port, got %02x", value);
        return;
    }
    case 18: {
        // in
        out("Sending byte %02x", level);
        hd44780_lcd__send_byte(lcd, level);
        out("Sent byte %02x", level);
        return;
    }
    case 19: {
        // in
        out("Selecting device %d", level);
        mios32_lcd_device = level;
        return;
    }

    }
}

/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_TerminalParseLine(char *input, void *_output_function)
{
    void (*out)(char *format, ...) = _output_function;
    char *separators = " \t";
    char *brkt;
    char *parameter;

    // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
    // on an unsuccessful call (whenever this function returns < 1)
    int input_len = strlen(input);

    if( (parameter = strtok_r(input, separators, &brkt)) ) {
        if( strcasecmp(parameter, "testlcdpin") == 0 ) {
            char *arg;
            int pin_number = -1;
            int level = -1;

            if( (arg = strtok_r(NULL, separators, &brkt)) ) {
                if( strcasecmp(arg, "rs") == 0 )
                    pin_number = 1;
                else if( strcasecmp(arg, "e1") == 0 )
                    pin_number = 2;
                else if( strcasecmp(arg, "e2") == 0 )
                    pin_number = 3;
                else if( strcasecmp(arg, "rw") == 0 )
                    pin_number = 4;
                else if( strcasecmp(arg, "d0") == 0 )
                    pin_number = 5;
                else if( strcasecmp(arg, "d1") == 0 )
                    pin_number = 6;
                else if( strcasecmp(arg, "d2") == 0 )
                    pin_number = 7;
                else if( strcasecmp(arg, "d3") == 0 )
                    pin_number = 8;
                else if( strcasecmp(arg, "d4") == 0 )
                    pin_number = 9;
                else if( strcasecmp(arg, "d5") == 0 )
                    pin_number = 10;
                else if( strcasecmp(arg, "d6") == 0 )
                    pin_number = 11;
                else if( strcasecmp(arg, "d7") == 0 )
                    pin_number = 12;

                else if( strcasecmp(arg, "out") == 0 )
                    pin_number = 14;
                else if( strcasecmp(arg, "init") == 0 )
                    pin_number = 15;
                else if( strcasecmp(arg, "char") == 0 )
                    pin_number = 16;
                else if( strcasecmp(arg, "in") == 0 )
                    pin_number = 17;
                else if( strcasecmp(arg, "byte") == 0 )
                    pin_number = 18;
                else if( strcasecmp(arg, "dev") == 0 )
                    pin_number = 19;



                else if( strcasecmp(arg, "reset") == 0 ) {
                    pin_number = 0;
                    level = 0; // dummy
                }
            }

            if( pin_number < 0 ) {
                out("Please specifiy valid LCD pin name: rs, e1, e2, rw, d0, d1, ... d7\n");
            }

            if( (arg = strtok_r(NULL, separators, &brkt)) )
                level = get_dec(arg);

            if( level != 0 && level != 1 ) {
                out("Please specifiy valid logic level for LCD pin: 0 or 1\n");
            }

            if( pin_number >= 0 && level >= 0 ) {
                test_mode__set_lcd_pin_and_report(&displays[mios32_lcd_device], pin_number, level, _output_function);
            } else {
                out("Following commands are supported:\n");
                out("testlcdpin rs 0  -> sets J15(AB):RS to ca. 0V");
                out("testlcdpin rs 1  -> sets J15(AB):RS to ca. 3.3V");
                out("testlcdpin e1 0  -> sets J15A:E to ca. 0V");
                out("testlcdpin e1 1  -> sets J15A:E to ca. 3.3V");
                out("testlcdpin e2 0  -> sets J15B:E to ca. 0V");
                out("testlcdpin e2 1  -> sets J15B:E to ca. 3.3V");
                out("testlcdpin rw 0  -> sets J15(AB):RW to ca. 0V");
                out("testlcdpin rw 1  -> sets J15(AB):RW to ca. 3.3V");
                out("testlcdpin d0 0  -> sets J15(AB):D0 to ca. 0V");
                out("testlcdpin d0 1  -> sets J15(AB):D0 to ca. 5V (resp. 3.3V)");
                out("testlcdpin d1 0  -> sets J15(AB):D1 to ca. 0V");
                out("testlcdpin d1 1  -> sets J15(AB):D1 to ca. 5V (resp. 3.3V)");
                out("testlcdpin d...  -> same for J15(AB):D2, D3, D4, D5, D6, D7");
                out("testlcdpin reset -> re-initializes LCD modules so that they can be used again.");
            }
            return 1; // command taken
        }
    }

    // restore input line (replace NUL characters by spaces)
    int i;
    char *input_ptr = input;
    for(i=0; i<input_len; ++i, ++input_ptr)
        if( !*input_ptr )
            *input_ptr = ' ';

    return 0; // command not taken
}


/////////////////////////////////////////////////////////////////////////////
// Prints the current configuration
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_TerminalPrintConfig(void *_output_function) {
    void (*out)(char *format, ...) = _output_function;
    out("Unsupported\n");
    return 0; // no error
}
