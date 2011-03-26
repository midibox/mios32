//*****************************************************************************
//   +--+
//   | ++----+
//   +-++    |
//     |     |
//   +-+--+  |
//   | +--+--+
//   +----+    Copyright (c) 2010 Code Red Technologies Ltd.
//
// Wrapper for LPC178x.h header (added due to name change)
//
// Software License Agreement
//
// The software is owned by Code Red Technologies and/or its suppliers, and is
// protected under applicable copyright laws.  All rights are reserved.  Any
// use in violation of the foregoing restrictions may subject the user to criminal
// sanctions under applicable laws, as well as to civil liability for the breach
// of the terms and conditions of this license.
//
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// USE OF THIS SOFTWARE FOR COMMERCIAL DEVELOPMENT AND/OR EDUCATION IS SUBJECT
// TO A CURRENT END USER LICENSE AGREEMENT (COMMERCIAL OR EDUCATIONAL) WITH
// CODE RED TECHNOLOGIES LTD.
//
//*****************************************************************************
#ifndef CR_LPC177x_8x_H_
#define CR_LPC177x_8x_H_

// NXP have changed the name of the CMSIS header files for LPC177x / LPC178x
// parts. This header is provided to act as a wrapper such that code that
// uses the old header filename will redirect automatically to the new
// header. A warning message will also be emitted.
#warning ** Using wrapper header - change your code to include LPC178x.h directly **
#include "LPC178x.h"

#endif /* CR_LPC177x_8x_H_ */

