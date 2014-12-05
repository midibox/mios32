// $Id$
/*
 * Header file for SPI MIDI functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_SPI_MIDI_H
#define _MIOS32_SPI_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// how many SPI MIDI ports are available?
// if 0: interface disabled (default)
// other allowed values: 1..8
#ifndef MIOS32_SPI_MIDI_NUM_PORTS
#define MIOS32_SPI_MIDI_NUM_PORTS 0
#endif

// Which SPI peripheral should be used
// allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
#ifndef MIOS32_SPI_MIDI_SPI
#define MIOS32_SPI_MIDI_SPI 0
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef MIOS32_SPI_MIDI_SPI_RC_PIN
#define MIOS32_SPI_MIDI_SPI_RC_PIN 1
#endif

// Which transfer rate should be used?
// MIOS32_SPI_PRESCALER_16 typically results into ca. 5 MBit/s
#ifndef MIOS32_SPI_MIDI_SPI_PRESCALER
#define MIOS32_SPI_MIDI_SPI_PRESCALER MIOS32_SPI_PRESCALER_16
#endif

// DMA buffer size
// Note: should match with transfer rate
// e.g. with MIOS32_SPI_PRESCALER_64 (typically ca. 2 MBit) the transfer of 1 word takes ca. 18 uS
// MIOS32_SPI_MIDI_Tick() is serviced each mS
// Accordingly, we shouldn't send more than 55 words, taking 50 seems to be a good choice.
#ifndef MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE
#define MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE 16
#endif
// note also, that the resulting RAM consumption will be 4*4*MIOS32_SPI_MIDI_BUFFER_SIZE bytes (Rx/Tx double buffers for 4byte words)

// we've a separate RX ringbuffer which stores received MIDI events until the
// application processes them
#ifndef MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE
#define MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE 64
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_SPI_MIDI_Init(u32 mode);

extern s32 MIOS32_SPI_MIDI_Enabled(void);

extern s32 MIOS32_SPI_MIDI_CheckAvailable(u8 spi_midi_port);

extern s32 MIOS32_SPI_MIDI_Periodic_mS(void);

extern s32 MIOS32_SPI_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package);
extern s32 MIOS32_SPI_MIDI_PackageSend(mios32_midi_package_t package);
extern s32 MIOS32_SPI_MIDI_PackageReceive(mios32_midi_package_t *package);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_SPI_MIDI_H */
