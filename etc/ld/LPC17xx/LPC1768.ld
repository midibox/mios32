/* $Id$
Linker script for LPC1768
*/

/* Memory Spaces Definitions */
MEMORY
{
  RAM (xrw) :     ORIGIN = 0x10000000, LENGTH = 32K
  RAMAHB (xrw) :  ORIGIN = 0x2007c000, LENGTH = 32K
  FLASH (rx) :    ORIGIN = 0x00000000, LENGTH = 512K
}

/* higher address of the user mode stack */
_estack = 0x10008000;


/* default stack sizes. 

These are used by the startup in order to allocate stacks for the different modes.

Note: FreeRTOS gives each task an own stack, the default stack sizes used by the
"programming model" can be changed with MIOS32_MINIMAL_STACK_SIZE
*/

__Stack_Size = 128 ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;

__Stack_Init = _estack  - __Stack_Size ;

/*"PROVIDE" allows to easily override these values from an object file or the commmand line.*/
PROVIDE ( _Stack_Init = __Stack_Init ) ;

/*
There will be a link error if there is not this amount of RAM free at the end.
*/
_Minimum_Stack_Size = 0x100 ;



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
                /* This is used by the startup in order to initialize the .data section */
                _sdata = . ;
                
                . = ALIGN(0x80);
                _isr_vectorsram_offs = . - 0x10000000;
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
                __end = .;
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
    } >RAM
    

   
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



