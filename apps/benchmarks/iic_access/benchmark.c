// $Id$
/*
 * Benchmark for IIC accesses
 * See README.txt for details
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "benchmark.h"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Init(u32 mode)
{
  s32 status = 0;

  // initialize IIC interface
  status |= MIOS32_IIC_Init(0);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// this function resets the benchmark
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Reset(void)
{
  s32 status;

  // try to get the IIC peripheral
  if( (status=MIOS32_IIC_TransferBegin(IIC_Blocking)) < 0 )
    return status; // request a retry

  // try to access EEPROM
  status = MIOS32_IIC_Transfer(IIC_Write, MIOS32_IIC_BS_ADDR_BASE, NULL, 0);
  if( status >= 0 )
    status = MIOS32_IIC_TransferWait();

  // release IIC peripheral
  MIOS32_IIC_TransferFinished();

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// this function performs the benchmark for reads
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Start_Reads(void)
{
  // perform 1000 reads consisting of 1 IIC address byte, 2 EEPROM address bytes and 1 read data
  u16 address = 0x1234;
  u8 read_buffer;
  u8 addr_buffer[2] = {(u8)(address>>8), (u8)address};
  s32 status;
  int i;

  for(i=0; i<1000; ++i) {
    if( (status=MIOS32_IIC_TransferBegin(IIC_Blocking)) < 0 )
      return status;

    if( (status=MIOS32_IIC_Transfer(IIC_Write_WithoutStop, MIOS32_IIC_BS_ADDR_BASE, addr_buffer, 2)) < 0 )
      return status;

    if( (status=MIOS32_IIC_TransferWait()) < 0 )
      return status;

    if( (status=MIOS32_IIC_Transfer(IIC_Read, MIOS32_IIC_BS_ADDR_BASE, &read_buffer, 1)) < 0 )
      return status;

    if( (status=MIOS32_IIC_TransferWait()) < 0 )
      return status;

    MIOS32_IIC_TransferFinished();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this function performs the benchmark for block reads
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Start_BlockReads(void)
{
  // perform 1000 block reads consisting of 1 IIC address byte, 2 EEPROM address bytes and 64 data bytes
  u16 address = 0x1200; // better to align the address
  u8 read_buffer[64];
  u8 addr_buffer[2] = {(u8)(address>>8), (u8)address};
  s32 status;
  int i;

  for(i=0; i<1000; ++i) {
    if( (status=MIOS32_IIC_TransferBegin(IIC_Blocking)) < 0 )
      return status;

    if( (status=MIOS32_IIC_Transfer(IIC_Write_WithoutStop, MIOS32_IIC_BS_ADDR_BASE, addr_buffer, 2)) < 0 )
      return status;

    if( (status=MIOS32_IIC_TransferWait()) < 0 )
      return status;

    if( (status=MIOS32_IIC_Transfer(IIC_Read, MIOS32_IIC_BS_ADDR_BASE, read_buffer, 64)) < 0 )
      return status;

    if( (status=MIOS32_IIC_TransferWait()) < 0 )
      return status;

    MIOS32_IIC_TransferFinished();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this function performs the benchmark for writes
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Start_Writes(void)
{
  // perform 1000 writes consisting of 1 IIC address byte and 2 EEPROM address bytes
  // no data byte! (EEPROM won't be overwritten)
  u16 address = 0x1234;
  u8 addr_buffer[2] = {(u8)(address>>8), (u8)address};
  s32 status;
  int i;

  for(i=0; i<1000; ++i) {
    if( (status=MIOS32_IIC_TransferBegin(IIC_Blocking)) < 0 )
      return status;

    if( (status=MIOS32_IIC_Transfer(IIC_Write_WithoutStop, MIOS32_IIC_BS_ADDR_BASE, addr_buffer, 2)) < 0 )
      return status;

    if( (status=MIOS32_IIC_TransferWait()) < 0 )
      return status;

    MIOS32_IIC_TransferFinished();
  }

  return 0; // no error
}
