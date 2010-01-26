/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Parameter Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_PAR_H
#define _MB_SID_PAR_H

#include <mios32.h>
#include "MbSidStructs.h"
//#include "MbSidSe.h"
class MbSidSe; // forward declaration, see also http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.11

typedef struct mbsid_par_table_item_t {
    u8 left_string;
    u8 right_string;
    u8 mod_function;
    u8 addr_l;
} mbsid_par_table_item_t;


class MbSidPar
{
public:
    // Constructor
    MbSidPar();

    // Destructor
    ~MbSidPar();

    // references
    MbSidSe *mbSidSePtr;

    // set functions
    void parSet(u8 par, u16 value, u8 sidlr, u8 ins);
    void parSet16(u8 par, u16 value, u8 sidlr, u8 ins);
    void parSetNRPN(u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins);
    void parSetWT(u8 par, u8 wt_value, u8 sidlr, u8 ins);

    // get functions
    u16 parGet(u8 par, u8 sidlr, u8 ins, u8 shadow);

private:
    void parHlpNote(u8 value, u8 voice_sel);
    u16 parScale(mbsid_par_table_item_t *item, u16 value, u8 scale_up);

};

#endif /* _MB_SID_PAR_H */
