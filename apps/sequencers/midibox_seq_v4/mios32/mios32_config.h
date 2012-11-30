// $Id$
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H


// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_DELAY 0 // we delay the boot and print a message inside the app
//                                <---------------------->
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox SEQ V4.068"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2012 T. Klose"

// USB settings
#define MIOS32_USB_PRODUCT_STR  "MIDIbox SEQ V4"
#define MIOS32_USB_PRODUCT_ID     1022
#define MIOS32_USB_MIDI_NUM_PORTS 4

// port used for debugging via MIDI
//#define MIOS32_MIDI_DEBUG_PORT USB0

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// Stack size for FreeRTOS tasks as defined by the programming model
// Note that each task maintains it's own stack!
// If you want to define a different stack size for your application tasks
// (-> xTaskCreate() function), keep in mind that it has to be divided by 4,
// since the stack width of ARM is 32bit.
// The FreeRTOS define "configMINIMAL_STACK_SIZE" is (MIOS32_MINIMAL_STACK_SIZE/4)
// it can be used in applications as well, e.g.
// xTaskCreate(TASK_Period1mS, (signed portCHAR *)"Period1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, NULL);
#define MIOS32_MINIMAL_STACK_SIZE 1100
// P.S.: in order to check if the stack size is sufficient, store a preset pattern in Event->Presets page
// Sequencer could crash with hardfault on a buffer overrun

// reserved memory for FreeRTOS pvPortMalloc function
#define MIOS32_HEAP_SIZE 14*1024

// for LPC17: simplify allocation of large arrays
#if defined(MIOS32_FAMILY_LPC17xx)
# define AHB_SECTION __attribute__ ((section (".bss_ahb")))
#else
# define AHB_SECTION
#endif


// to save some RAM (only 128 bytes, but "Kleinvieh macht auch Mist" - especially for LPC17)
#define MIOS32_ENC_NUM_MAX 32


// optional performance measuring
// see documentation under http://www.midibox.org/mios32/manual/group___f_r_e_e_r_t_o_s___u_t_i_l_s.html
#define configGENERATE_RUN_TIME_STATS           0
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  FREERTOS_UTILS_PerfCounterInit
#define portGET_RUN_TIME_COUNTER_VALUE          FREERTOS_UTILS_PerfCounterGet


// maximum idle counter value to be expected
#if defined(MIOS32_FAMILY_LPC17xx)
#define MAX_IDLE_CTR 908500 // LPC1769@120 MHz
#else
#define MAX_IDLE_CTR 228000 // STM32F103RE@80 MHz
#endif


// memory alloccation method:
// 0: internal static allocation with one byte for each flag
// 1: internal static allocation with 8bit flags
// 2: internal static allocation with 16bit flags
// 3: internal static allocation with 32bit flags
// 4: FreeRTOS based pvPortMalloc
// 5: malloc provided by library
#define SEQ_MIDI_OUT_MALLOC_METHOD 3

// max number of scheduled events which will allocate memory
// each event allocates 12 bytes
// MAX_EVENTS must be a power of two! (e.g. 64, 128, 256, 512, ...)
#define SEQ_MIDI_OUT_MAX_EVENTS 256

// enable seq_midi_out_max_allocated and seq_midi_out_dropouts
#define SEQ_MIDI_OUT_MALLOC_ANALYSIS 1


#if defined(MIOS32_FAMILY_STM32F10x)
// enable third UART
# define MIOS32_UART_NUM 3
#else
// enable third and fourth UART
# define MIOS32_UART_NUM 4
#endif


// configure IIC_MIDI
#define MIOS32_IIC_MIDI_NUM 4
// all interfaces are "OUT only"
#define MIOS32_IIC_MIDI0_ENABLED    1
#define MIOS32_IIC_MIDI1_ENABLED    1
#define MIOS32_IIC_MIDI2_ENABLED    1
#define MIOS32_IIC_MIDI3_ENABLED    1
#define MIOS32_IIC_MIDI4_ENABLED    1
#define MIOS32_IIC_MIDI5_ENABLED    1
#define MIOS32_IIC_MIDI6_ENABLED    1
#define MIOS32_IIC_MIDI7_ENABLED    1


// configure BLM driver
#define BLM_DOUT_L1_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_R1_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_CATHODES_SR1	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_CATHODES_SR2	255 // dummy, will be changed in seq_file_hw.c
#define BLM_CATHODES_INV_MASK	0x00
#define BLM_DOUT_L2_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_R2_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_L3_SR	0 // not used
#define BLM_DOUT_R3_SR	0 // not used
#define BLM_DIN_L_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DIN_R_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_NUM_COLOURS 2
#define BLM_NUM_ROWS    8
#define BLM_DEBOUNCE_MODE 1


// configure BLM_X driver
#define BLM_X_NUM_ROWS            8
#define BLM_X_BTN_NUM_COLS        8
#define BLM_X_LED_NUM_COLS        8
#define BLM_X_LED_NUM_COLORS      1
#define BLM_X_ROWSEL_DOUT_SR      255 // dummy, will be changed in seq_file_hw.c
#define BLM_X_LED_FIRST_DOUT_SR   255 // dummy, will be changed in seq_file_hw.c
#define BLM_X_BTN_FIRST_DIN_SR    255 // dummy, will be changed in seq_file_hw.c
#define BLM_X_ROWSEL_INV_MASK     0   // dummy, will be changed in seq_file_hw.c
#define BLM_X_DEBOUNCE_MODE       1


// optional for Francois' 4 * 20x2 LCD configuration
// currently this option cannot be enabled in the MBSEQ_HW.V4 file - this will be possible later
#if 0
#define LCD_NUM_DEVICES          4
#define LCD_COLUMNS_PER_DEVICE  20
#define MIOS32_BOARD_LCD_E3_PORT GPIOB       // J15C.A10
#define MIOS32_BOARD_LCD_E3_PIN  GPIO_Pin_0

#define MIOS32_BOARD_LCD_E4_PORT GPIOB       // J15C.A11
#define MIOS32_BOARD_LCD_E4_PIN  GPIO_Pin_1
#endif

// LPC17 Ethernet driver: locate buffers to lower (default) section
// (no attribute passed to this variable)
#define LPC17XX_EMAC_MEM_SECTION
// reduce number of buffers to save memory
#define LPC17XX_EMAC_NUM_RX_FRAG 2
#define LPC17XX_EMAC_NUM_TX_FRAG 2
#define LPC17XX_EMAC_FRAG_SIZE   1024


// map MIDI mutex to UIP task
// located in tasks.c to access MIDI IN/OUT mutex from external
extern void TASKS_MUTEX_MIDIOUT_Take(void);
extern void TASKS_MUTEX_MIDIOUT_Give(void);
extern void TASKS_MUTEX_MIDIIN_Take(void);
extern void TASKS_MUTEX_MIDIIN_Give(void);
#define UIP_TASK_MUTEX_MIDIOUT_TAKE { TASKS_MUTEX_MIDIOUT_Take(); }
#define UIP_TASK_MUTEX_MIDIOUT_GIVE { TASKS_MUTEX_MIDIOUT_Give(); }
#define UIP_TASK_MUTEX_MIDIIN_TAKE  { TASKS_MUTEX_MIDIIN_Take(); }
#define UIP_TASK_MUTEX_MIDIIN_GIVE  { TASKS_MUTEX_MIDIIN_Give(); }

// Mutex for J16 access
extern void TASKS_J16SemaphoreTake(void);
extern void TASKS_J16SemaphoreGive(void);
#define MIOS32_SDCARD_MUTEX_TAKE   { TASKS_J16SemaphoreTake(); }
#define MIOS32_SDCARD_MUTEX_GIVE   { TASKS_J16SemaphoreGive(); }
#define MIOS32_ENC28J60_MUTEX_TAKE { TASKS_J16SemaphoreTake(); }
#define MIOS32_ENC28J60_MUTEX_GIVE { TASKS_J16SemaphoreGive(); }

#endif /* _MIOS32_CONFIG_H */
