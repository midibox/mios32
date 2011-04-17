/***********************************************************************
 * $Id::                                                               $
 *
 * Project: Common Include Files
 *
 * Description:
 *     lpc_types.h contains the NXP ABL typedefs for C standard types.
 *     It is intended to be used in ISO C conforming development
 *     environments and checks for this insofar as it is possible
 *     to do so.
 *
 *     lpc_types.h ensures that the name used to define types correctly
 *     identifies a representation size, and by direct inference the
 *     storage size, in bits. E.g., UNS_32 identifies an unsigned
 *     integer type stored in 32 bits.
 *
 *     It requires that the basic storage unit (char) be stored in
 *     8 bits.
 *
 *     No assumptions about Endianess are made or implied.
 *
 *     lpc_types.h also contains NXP ABL Global Macros:
 *         _BIT
 *         _SBF
 *         _BITMAP
 *     These #defines are not strictly types, but rather Preprocessor
 *     Macros that have been found to be generally useful.
 *
 ***********************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 **********************************************************************/

#ifndef LPC_TYPES_H
#define LPC_TYPES_H

/***********************************************************************
 * Global typedefs
 **********************************************************************/

/* SMA type for character type */
typedef char CHAR;

/* SMA type for 8 bit unsigned value */
typedef unsigned char UNS_8;

/* SMA type for 8 bit signed value */
typedef signed char INT_8;

/* SMA type for 16 bit unsigned value */
typedef	unsigned short UNS_16;

/* SMA type for 16 bit signed value */
typedef	signed short INT_16;

/* SMA type for 32 bit unsigned value */
typedef	unsigned int UNS_32;

/* SMA type for 32 bit signed value */
typedef	signed int INT_32;

/* SMA type for 64 bit signed value */
typedef long long INT_64;

/* SMA type for 64 bit unsigned value */
typedef unsigned long long UNS_64;

/* 32 bit boolean type */
typedef INT_32 BOOL_32;

/* 16 bit boolean type */
typedef INT_16 BOOL_16;

/* 8 bit boolean type */
typedef INT_8 BOOL_8;

/* Pointer to Function returning Void (any number of parameters) */
typedef void (*PFV)();

/* Pointer to Function returning INT_32 (any number of parameters) */
typedef INT_32(*PFI)();

/***********************************************************************
 * Global Macros
 **********************************************************************/

/* _BIT(n) sets the bit at position "n"
 * _BIT(n) is intended to be used in "OR" and "AND" expressions:
 * e.g., "(_BIT(3) | _BIT(7))".
 */
#undef _BIT
/* Set bit macro */
#define _BIT(n)	(((UNS_32)(1)) << (n))

/* _SBF(f,v) sets the bit field starting at position "f" to value "v".
 * _SBF(f,v) is intended to be used in "OR" and "AND" expressions:
 * e.g., "((_SBF(5,7) | _SBF(12,0xF)) & 0xFFFF)"
 */
#undef _SBF
/* Set bit field macro */
#define _SBF(f,v) (((UNS_32)(v)) << (f))

/* _BITMASK constructs a symbol with 'field_width' least significant
 * bits set.
 * e.g., _BITMASK(5) constructs '0x1F', _BITMASK(16) == 0xFFFF
 * The symbol is intended to be used to limit the bit field width
 * thusly:
 * <a_register> = (any_expression) & _BITMASK(x), where 0 < x <= 32.
 * If "any_expression" results in a value that is larger than can be
 * contained in 'x' bits, the bits above 'x - 1' are masked off.  When
 * used with the _SBF example above, the example would be written:
 * a_reg = ((_SBF(5,7) | _SBF(12,0xF)) & _BITMASK(16))
 * This ensures that the value written to a_reg is no wider than
 * 16 bits, and makes the code easier to read and understand.
 */
#undef _BITMASK
/* Bitmask creation macro */
#define _BITMASK(field_width) ( _BIT(field_width) - 1)

/* SUCCESS macro */
#define SUCCESS     0

#ifndef FALSE
/* FALSE macro */
#define FALSE (0==1)
#endif
#ifndef TRUE
/* TRUE macro */
#define TRUE (!(FALSE))
#endif
#if 0
	#ifndef TRUE
	#define TRUE  (1 == 1)
	#endif
#endif
/* NULL pointer */
#ifndef NULL
#define NULL ((void*) 0)
#endif

/* Number of elements in an array */
#define NELEMENTS(array)  (sizeof (array) / sizeof (array[0]))

/* Static data/function define */
#define STATIC static
/* External data/function define */
#define EXTERN extern
/* Status type */
typedef INT_32 STATUS;

/* NO_ERROR macro */
#define _NO_ERROR           (INT_32)(0)
/* ERROR macro */
#define _ERROR              (INT_32)(-1)
/* Device unknown macro */
#define LPC_DEV_UNKNOWN     (INT_32)(-2)
/* Device not supported macro */
#define LPC_NOT_SUPPORTED   (INT_32)(-3)
/* Device not open macro */
#define LPC_NOT_OPEN        (INT_32)(-4)
/* Device in use macro */
#define LPC_IN_USE          (INT_32)(-5)
/* Device oin conflict macro */
#define LPC_PIN_CONFLICT    (INT_32)(-6)
/* Device bad paramaters macro */
#define LPC_BAD_PARAMS      (INT_32)(-7)
/* Bad device handle macro */
#define LPC_BAD_HANDLE      (INT_32)(-8)
/* Bad device clock macro */
#define LPC_BAD_CLK         (INT_32)(-9)
/* Device can't start macro */
#define LPC_CANT_START      (INT_32)(-10)
/* Device can't stop macro */
#define LPC_CANT_STOP       (INT_32)(-11)

/* following are legacy defines which are OBSELETE. DONOT USE. */
#define SMA_DEV_UNKNOWN     LPC_DEV_UNKNOWN
#define SMA_NOT_SUPPORTED   LPC_NOT_SUPPORTED
#define SMA_NOT_OPEN        LPC_NOT_OPEN
#define SMA_IN_USE          LPC_IN_USE
#define SMA_PIN_CONFLICT    LPC_PIN_CONFLICT
#define SMA_BAD_PARAMS      LPC_BAD_PARAMS
#define SMA_BAD_HANDLE      LPC_BAD_HANDLE
#define SMA_BAD_CLK         LPC_BAD_CLK
#define SMA_CANT_START      LPC_CANT_START
#define SMA_CANT_STOP       LPC_CANT_STOP


#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#endif /* LPC_TYPES_H */
