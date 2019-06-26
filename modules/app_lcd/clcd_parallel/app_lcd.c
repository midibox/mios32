// $Id$
/*
 * Application specific LCD driver for up to 4 HD44870 LCDs,
 * connected via individual, parallel 8-wire data + 3-wire control buses.
 *
 * LCD DATA = PORT E 8..15
 * LCD RS   = PORT B 0
 * LCD RW   = PORT B 1
 * LCD E    = PORT B 2
 * ==========================================================================
 * When not working with LCD:
 * RS is 0 (command mode; don't remember why it was necessary),
 * RW is 0 (write; don't remember why it was necessary),
 * DATA pins are configured for input with or without pull-ups.
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

#define LCD0__RS__PORT          GPIOB
#define LCD0__RS__PIN           GPIO_Pin_0
#define LCD0__RW__PORT          GPIOB
#define LCD0__RW__PIN           GPIO_Pin_1
#define LCD0__E__PORT           GPIOB
#define LCD0__E__PIN            GPIO_Pin_2
#define LCD0__DATA__PORT        GPIOE
#define LCD0__DATA__PIN_OFFSET  8U
#define LCD0__DATA__PINS_MASK   (\
  (1U << LCD0__DATA__PIN_OFFSET)\
| (1U << (LCD0__DATA__PIN_OFFSET + 1))\
| (1U << (LCD0__DATA__PIN_OFFSET + 2))\
| (1U << (LCD0__DATA__PIN_OFFSET + 3))\
| (1U << (LCD0__DATA__PIN_OFFSET + 4))\
| (1U << (LCD0__DATA__PIN_OFFSET + 5))\
| (1U << (LCD0__DATA__PIN_OFFSET + 6))\
| (1U << (LCD0__DATA__PIN_OFFSET + 7))\
)

/////////////////////////////////////////////////////////////////////////////
// HD44780 stuff
/////////////////////////////////////////////////////////////////////////////

#define HD44780_LCD__TIMEOUT                        10000

#define HD44780_LCD__COMMAND__CLEAR                 0x01U
#define HD44780_LCD__COMMAND__USE_DDRAM             0x02U
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
#define HD44780_LCD__COMMAND__SET_DDRAM_ADDRESS     0x80U

#define HD44780_LCD__LINE0_ADDRESS 0x00U
#define HD44780_LCD__LINE1_ADDRESS 0x40U
#define HD44780_LCD__LINE2_ADDRESS 0x14U
#define HD44780_LCD__LINE3_ADDRESS 0x54U

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static unsigned long long display_available = 0;
GPIO_InitTypeDef lcd0__GPIO_InitStructure;


void hd44780_lcd__register_select__init(void) {
    lcd0__GPIO_InitStructure.GPIO_Pin = LCD0__RS__PIN;
    GPIO_Init(LCD0__RS__PORT, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__register_select__command(void) {
    MIOS32_SYS_STM_PINSET_0(LCD0__RS__PORT, LCD0__RS__PIN);
}

void hd44780_lcd__register_select__data(void) {
    MIOS32_SYS_STM_PINSET_1(LCD0__RS__PORT, LCD0__RS__PIN);
}


void hd44780_lcd__mode_init(void) {
    lcd0__GPIO_InitStructure.GPIO_Pin = LCD0__RW__PIN;
    GPIO_Init(LCD0__RW__PORT, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__mode_read(void) {
    MIOS32_SYS_STM_PINSET_1(LCD0__RW__PORT, LCD0__RW__PIN);
}

void hd44780_lcd__mode_write(void) {
    MIOS32_SYS_STM_PINSET_0(LCD0__RW__PORT, LCD0__RW__PIN);
}


void hd44780_lcd__e__init(void) {
    lcd0__GPIO_InitStructure.GPIO_Pin = LCD0__E__PIN;
    GPIO_Init(LCD0__E__PORT, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__e_high(void) {
    MIOS32_SYS_STM_PINSET_1(LCD0__E__PORT, LCD0__E__PIN);
}

void hd44780_lcd__e_low(void) {
    MIOS32_SYS_STM_PINSET_0(LCD0__E__PORT, LCD0__E__PIN);
}

void hd44780_lcd__latch_data(void) {
    hd44780_lcd__e_high();
    MIOS32_DELAY_Wait_uS(1);
    hd44780_lcd__e_low();
}


void hd44780_lcd__data_port__mode_write(void) {
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    lcd0__GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LCD0__DATA__PORT, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__data_port__mode_read(void) {
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    lcd0__GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(LCD0__DATA__PORT, &lcd0__GPIO_InitStructure);
}

void hd44780_lcd__data_port__mode_hi(void) {
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    lcd0__GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LCD0__DATA__PORT, &lcd0__GPIO_InitStructure);
}


void hd44780_lcd__data_port__write(const uint8_t value) {
    LCD0__DATA__PORT->ODR = (uint16_t) (value << LCD0__DATA__PIN_OFFSET)
        | (LCD0__DATA__PORT->ODR & ~LCD0__DATA__PINS_MASK);
}

uint8_t hd44780_lcd__is_busy(void) {
    return MIOS32_SYS_STM_PINGET(LCD0__DATA__PORT, 1U << LCD0__DATA__PIN_OFFSET);
}

void hd44780_lcd__wait_until_not_busy(u32 time_out) {
    hd44780_lcd__data_port__mode_read();
    hd44780_lcd__mode_read();
    while (time_out > 0) {
        hd44780_lcd__e_high();
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

        if (!hd44780_lcd__is_busy()) {
            time_out = 0;
        }

        hd44780_lcd__e_low();
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
    }
    hd44780_lcd__mode_write();
    // data pins remain in read mode
}


void hd44780_lcd__send_byte(const uint8_t i) {
    hd44780_lcd__data_port__mode_write();
    hd44780_lcd__data_port__write(i);
    hd44780_lcd__latch_data();
    hd44780_lcd__data_port__mode_hi();
}

void hd44780_lcd__send_command(const uint8_t i) {
    hd44780_lcd__wait_until_not_busy(HD44780_LCD__TIMEOUT);
    hd44780_lcd__send_byte(i);
}

void hd44780_lcd__send_data(const uint8_t i) {
    hd44780_lcd__wait_until_not_busy(HD44780_LCD__TIMEOUT);
    hd44780_lcd__register_select__data();
    hd44780_lcd__send_byte(i);
    hd44780_lcd__register_select__command();
}



void hd44780_lcd__init(void) {
    GPIO_StructInit(&lcd0__GPIO_InitStructure);
    lcd0__GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    lcd0__GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    lcd0__GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; // weak driver to reduce transients

    hd44780_lcd__e__init();
    hd44780_lcd__e_low();

    hd44780_lcd__register_select__init();
    hd44780_lcd__register_select__command();

    hd44780_lcd__mode_init();
    hd44780_lcd__mode_write();

    lcd0__GPIO_InitStructure.GPIO_Pin = LCD0__DATA__PINS_MASK;

    uint8_t i = 0;
    while (i != 3) {
        hd44780_lcd__data_port__write(HD44780_LCD__COMMAND__CONFIGURE | HD44780_LCD__COMMAND__CONFIGURE_BUS_8_BIT);
        hd44780_lcd__latch_data();
        MIOS32_DELAY_Wait_uS(5000);
        i++;
    }

    hd44780_lcd__send_command(
        HD44780_LCD__COMMAND__CONFIGURE |
        HD44780_LCD__COMMAND__CONFIGURE_BUS_8_BIT |
        HD44780_LCD__COMMAND__CONFIGURE_LINES_2 |
        HD44780_LCD__COMMAND__CONFIGURE_CHAR_5X8
    );
    hd44780_lcd__send_command(HD44780_LCD__COMMAND__DISPLAY | HD44780_LCD__COMMAND__DISPLAY_ON | HD44780_LCD__COMMAND__DISPLAY_CURSOR_OFF);
    hd44780_lcd__send_command(HD44780_LCD__COMMAND__SCREEN | HD44780_LCD__COMMAND__SCREEN_ADDRESS_INC);
    hd44780_lcd__send_command(HD44780_LCD__COMMAND__USE_DDRAM);
    hd44780_lcd__send_command(HD44780_LCD__COMMAND__CLEAR);
}


void hd44780_lcd__goto(const uint8_t x, const uint8_t y) {
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

    hd44780_lcd__send_command(command);
}


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode) {
    hd44780_lcd__init();
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data) {
    hd44780_lcd__send_data(data);
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd) {
    hd44780_lcd__send_command(cmd);
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void) {
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line) {
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
    return -3; // not supported
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
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_TerminalParseLine(char *input, void *_output_function) {
    void (*out)(char *format, ...) = _output_function;
    out("Unsupported\n");
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
