/* $Id: STM32F103_bsl.ld 1599 2012-12-13 00:18:38Z tk $
Linker script for STM32F103RE
Based on scripts from Raisonance
*/

/* Memory Spaces Definitions */
MEMORY
{
  RAM (xrw) :     ORIGIN = 0x20000000, LENGTH = 20K
  FLASH (rx) :    ORIGIN = 0x08000000, LENGTH = 0x3f00
  FLASHB1 (rx) :  ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB0 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB2 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB3 (rx) : ORIGIN = 0x00000000, LENGTH = 0
}

/* higher address of the user mode stack */
_estack = 0x20005000;


/* default stack sizes. 

These are used by the startup in order to allocate stacks for the different modes.

*/

__Stack_Size = 1024 ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;

__Stack_Init = _estack  - __Stack_Size ;

/*"PROVIDE" allows to easily override these values from an object file or the commmand line.*/
PROVIDE ( _Stack_Init = __Stack_Init ) ;

/*
There will be a link error if there is not this amount of RAM free at the end.
*/
_Minimum_Stack_Size = 0x100 ;


/******************************************************************************/
/*                       Peripheral memory map                                */
/******************************************************************************/
/*this allows to compile the ST lib in "non-debug" mode*/


/* Peripheral and SRAM base address in the alias region */
PERIPH_BB_BASE        = 0x42000000;
SRAM_BB_BASE          = 0x22000000;

/* Peripheral and SRAM base address in the bit-band region */
SRAM_BASE             = 0x20000000;
PERIPH_BASE           = 0x40000000;

/* Flash registers base address */
PROVIDE ( FLASH_BASE            = 0x40022000);
/* Flash Option Bytes base address */
PROVIDE ( OB_BASE               = 0x1FFFF800);

/* Peripheral memory map */
APB1PERIPH_BASE      = PERIPH_BASE  ;
APB2PERIPH_BASE      = (PERIPH_BASE + 0x10000) ;
AHBPERIPH_BASE       = (PERIPH_BASE + 0x20000) ;

PROVIDE ( TIM2            = (APB1PERIPH_BASE + 0x0000) ) ;
PROVIDE ( TIM3            = (APB1PERIPH_BASE + 0x0400) ) ;
PROVIDE ( TIM4            = (APB1PERIPH_BASE + 0x0800) ) ;
PROVIDE ( RTC             = (APB1PERIPH_BASE + 0x2800) ) ;
PROVIDE ( WWDG            = (APB1PERIPH_BASE + 0x2C00) ) ;
PROVIDE ( IWDG            = (APB1PERIPH_BASE + 0x3000) ) ;
PROVIDE ( SPI2            = (APB1PERIPH_BASE + 0x3800) ) ;
PROVIDE ( USART2          = (APB1PERIPH_BASE + 0x4400) ) ;
PROVIDE ( USART3          = (APB1PERIPH_BASE + 0x4800) ) ;
PROVIDE ( I2C1            = (APB1PERIPH_BASE + 0x5400) ) ;
PROVIDE ( I2C2            = (APB1PERIPH_BASE + 0x5800) ) ;
PROVIDE ( CAN             = (APB1PERIPH_BASE + 0x6400) ) ;
PROVIDE ( BKP             = (APB1PERIPH_BASE + 0x6C00) ) ;
PROVIDE ( PWR             = (APB1PERIPH_BASE + 0x7000) ) ;

PROVIDE ( AFIO            = (APB2PERIPH_BASE + 0x0000) ) ;
PROVIDE ( EXTI            = (APB2PERIPH_BASE + 0x0400) ) ;
PROVIDE ( GPIOA           = (APB2PERIPH_BASE + 0x0800) ) ;
PROVIDE ( GPIOB           = (APB2PERIPH_BASE + 0x0C00) ) ;
PROVIDE ( GPIOC           = (APB2PERIPH_BASE + 0x1000) ) ;
PROVIDE ( GPIOD           = (APB2PERIPH_BASE + 0x1400) ) ;
PROVIDE ( GPIOE           = (APB2PERIPH_BASE + 0x1800) ) ;
PROVIDE ( ADC1            = (APB2PERIPH_BASE + 0x2400) ) ;
PROVIDE ( ADC2            = (APB2PERIPH_BASE + 0x2800) ) ;
PROVIDE ( TIM1            = (APB2PERIPH_BASE + 0x2C00) ) ;
PROVIDE ( SPI1            = (APB2PERIPH_BASE + 0x3000) ) ;
PROVIDE ( USART1          = (APB2PERIPH_BASE + 0x3800) ) ;

PROVIDE ( DMA             = (AHBPERIPH_BASE + 0x0000) ) ;
PROVIDE ( DMA_Channel1    = (AHBPERIPH_BASE + 0x0008) ) ;
PROVIDE ( DMA_Channel2    = (AHBPERIPH_BASE + 0x001C) ) ;
PROVIDE ( DMA_Channel3    = (AHBPERIPH_BASE + 0x0030) ) ;
PROVIDE ( DMA_Channel4    = (AHBPERIPH_BASE + 0x0044) ) ;
PROVIDE ( DMA_Channel5    = (AHBPERIPH_BASE + 0x0058) ) ;
PROVIDE ( DMA_Channel6    = (AHBPERIPH_BASE + 0x006C) ) ;
PROVIDE ( DMA_Channel7    = (AHBPERIPH_BASE + 0x0080) ) ;
PROVIDE ( RCC             = (AHBPERIPH_BASE + 0x1000) ) ;

/* System Control Space memory map */
SCS_BASE              = 0xE000E000;

PROVIDE ( SysTick         = (SCS_BASE + 0x0010) ) ;
PROVIDE ( NVIC            = (SCS_BASE + 0x0100) ) ;
PROVIDE ( SCB             = (SCS_BASE + 0x0D00) ) ;


/* Sections Definitions */

SECTIONS
{
    /* the MIOS32 BSL section */
    .mios32_bsl :
    {
	. = ALIGN(4);
        KEEP(*(.mios32_bsl))
	. = ALIGN(4);
    } >FLASH
 
    /* for Cortex devices, the beginning of the startup code is stored in the .isr_vector section, which goes to FLASH */
    .isr_vector :
    {
	_mios32_sys_isr_vector = . ;
	. = ALIGN(4);
        KEEP(*(.isr_vector))            /* Startup code */
	. = ALIGN(4);
    } >FLASH

    /* used in MIOS32_SYS to set the start vector */
    PROVIDE ( mios32_sys_isr_vector = _mios32_sys_isr_vector );
 
    /* for some STRx devices, the beginning of the startup code is stored in the .flashtext section, which goes to FLASH */
    .flashtext :
    {
	. = ALIGN(4);
        *(.flashtext)            /* Startup code */
	. = ALIGN(4);
    } >FLASH
 
    
       /* the program code is stored in the .text section, which goes to Flash */
        .text :
        {
                . = ALIGN(4);
                CREATE_OBJECT_SYMBOLS
                *(.text .text.* .gnu.linkonce.t.*)
                *(.plt)
                *(.gnu.warning)
                *(.glue_7t) *(.glue_7) *(.vfp11_veneer)
                *(.ARM.extab* .gnu.linkonce.armextab.*)
                *(.gcc_except_table)
        } >FLASH

        .eh_frame_hdr : ALIGN (4)
        {
                KEEP (*(.eh_frame_hdr))
        } >FLASH
        .eh_frame : ALIGN (4)
        {
                KEEP (*(.eh_frame))
        } >FLASH
        
        /* .ARM.exidx is sorted, so has to go in its own output section.  */
        __exidx_start = .;
        .ARM.exidx :
        {
                *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        } >FLASH
        __exidx_end = .;

        .rodata : ALIGN (4)
        {
                *(.rodata .rodata.* .gnu.linkonce.r.*)
                
                . = ALIGN(4);
                KEEP(*(.init))
                
                . = ALIGN(4);
                __preinit_array_start = .;
                KEEP (*(.preinit_array))
                __preinit_array_end = .;
                
                . = ALIGN(4);
                __init_array_start = .;
                KEEP (*(SORT(.init_array.*)))
                KEEP (*(.init_array))
                __init_array_end = .;
                
                . = ALIGN(4);
                KEEP(*(.fini))
                
                . = ALIGN(4);
                __fini_array_start = .;
                KEEP (*(.fini_array))
                KEEP (*(SORT(.fini_array.*)))
                __fini_array_end = .;
                
                . = ALIGN(0x4);
                KEEP (*crtbegin.o(.ctors))
                KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
                KEEP (*(SORT(.ctors.*)))
                KEEP (*crtend.o(.ctors))
                
                . = ALIGN(0x4);
                KEEP (*crtbegin.o(.dtors))
                KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
                KEEP (*(SORT(.dtors.*)))
                KEEP (*crtend.o(.dtors))
                
                *(.init .init.*)
                *(.fini .fini.*)
                
                PROVIDE_HIDDEN (__preinit_array_start = .);
                KEEP (*(.preinit_array))
                PROVIDE_HIDDEN (__preinit_array_end = .);
                PROVIDE_HIDDEN (__init_array_start = .);
                KEEP (*(SORT(.init_array.*)))
                KEEP (*(.init_array))
                PROVIDE_HIDDEN (__init_array_end = .);
                PROVIDE_HIDDEN (__fini_array_start = .);
                KEEP (*(.fini_array))
                KEEP (*(SORT(.fini_array.*)))
                PROVIDE_HIDDEN (__fini_array_end = .);
                
                . = ALIGN (8);
                *(.rom)
                *(.rom.b)
                _etext = .;
                /* This is used by the startup in order to initialize the .data secion */
                _sidata = _etext;
        } >FLASH

        /* This is the initialized data section
        The program executes knowing that the data is in the RAM
        but the loader puts the initial values in the FLASH (inidata).
        It is one task of the startup to copy the initial values from FLASH to RAM. */
        .data : ALIGN (8)
        {
                __ram_start = .;
                /* This is used by the startup in order to initialize the .data section */
                _sdata = . ;
                
                . = ALIGN(0x80);
                _isr_vectorsram_offs = . - 0x20000000;
                KEEP(*(.isr_vectorsram))
                
                . = ALIGN(4);
                KEEP(*(.jcr))
                *(.got.plt) *(.got)
                *(.shdata)
                *(.data .data.* .gnu.linkonce.d.*)
                . = ALIGN (8);
                *(.ram)
                *(.ramfunc*)
                 . = ALIGN(4);
                /* This is used by the startup in order to initialize the .data section */
                _edata = .;
        } >RAM AT>FLASH

        /* This is the uninitialized data section */
        .bss (NOLOAD):
        {
                . = ALIGN(4);
                /* This is used by the startup in order to initialize the .bss section */
                _sbss = .;
                *(.shbss)
                *(.bss .bss.* .gnu.linkonce.b.*)
                *(COMMON)
                . = ALIGN (8);
                *(.ram.b)
                . = ALIGN(4);
                 /* This is used by the startup in order to initialize the .bss section */
                 _ebss = . ;
                _end = .;
        } >RAM AT>FLASH

    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );
    PROVIDE ( __bss_start__ = _sbss );
    PROVIDE ( __bss_end__ = _ebss );


    /* This is the user stack section 
    This is just to check that there is enough RAM left for the User mode stack
    It should generate an error if it's full.
     */
    ._usrstack (NOLOAD):
    {
        . = ALIGN(4);
        _susrstack = . ;
        . = . + _Minimum_Stack_Size ;
        . = ALIGN(4);
        _eusrstack = . ;
        __ram_end = .;
    } >RAM
    

   
    /* this is the FLASH Bank1 */
    /* the C or assembly source must explicitly place the code or data there
    using the "section" attribute */
    .b1text :
    {
        *(.b1text)                   /* remaining code */
        *(.b1rodata)                 /* read-only data (constants) */
        *(.b1rodata*)
    } >FLASHB1
    
    /* this is the EXTMEM */
    /* the C or assembly source must explicitly place the code or data there
    using the "section" attribute */
    
    /* EXTMEM Bank0 */
    .eb0text :
    {
        *(.eb0text)                   /* remaining code */
        *(.eb0rodata)                 /* read-only data (constants) */
        *(.eb0rodata*)
    } >EXTMEMB0
    
    /* EXTMEM Bank1 */
    .eb1text :
    {
        *(.eb1text)                   /* remaining code */
        *(.eb1rodata)                 /* read-only data (constants) */
        *(.eb1rodata*)
    } >EXTMEMB1
    
    /* EXTMEM Bank2 */
    .eb2text :
    {
        *(.eb2text)                   /* remaining code */
        *(.eb2rodata)                 /* read-only data (constants) */
        *(.eb2rodata*)
    } >EXTMEMB2
    
    /* EXTMEM Bank0 */
    .eb3text :
    {
        *(.eb3text)                   /* remaining code */
        *(.eb3rodata)                 /* read-only data (constants) */
        *(.eb3rodata*)
    } >EXTMEMB3
    
    /* after that it's only debugging information. */
    
    /* remove the debugging information from the standard libraries */
    /DISCARD/ :
    {
     libc.a ( * )
     libm.a ( * )
     libgcc.a ( * )
     }

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}



