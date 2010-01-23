// $Id$
//////////////////////////////////////////////////////////////////////////////
// Minimal Embedded C++ support, no exception handling, no RTTI
// Date of the Last Update:  Jun 15, 2007
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2007 Quantum Leaps, LLC. All rights reserved.
//
// Contact information:
// Quantum Leaps Web site:  http://www.quantum-leaps.com
// e-mail:                  info@quantum-leaps.com
//////////////////////////////////////////////////////////////////////////////
// very minor modification to avoid warnings by Martin Thomas 12/2009
// Linking with the object-code from this file saves around 20kB program-memory
// in a demo-application for a Cortex-M3 (thumb2, CS G++ lite Q1/2009).
// Further information can be found in the documents from Quantum Leaps.
//////////////////////////////////////////////////////////////////////////////
// another very minor modification by Thorsten Klose (2010-01)
// just added comments to new() and delete(), that the versions of
// freertos_heap.cpp are used
//////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>                   // for prototypes of malloc() and free()

//............................................................................
void *operator new(size_t size) throw() {
	return malloc(size);
}
//............................................................................
void operator delete(void *p) throw() {
	free(p);
}
//............................................................................
extern "C" int __aeabi_atexit(void *object,
                              void (*destructor)(void *),
                              void *dso_handle)
{
	// avoid "unused" warnings (mthomas):
	object = object; destructor=destructor; dso_handle=dso_handle;

	return 0;
}
