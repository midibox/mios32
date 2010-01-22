/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#ifndef _MB_SID_H
#define _MB_SID_H

#include <mios32.h>

class MbSid
{
public:
    // Constructor
    MbSid();

    // Destructor
    ~MbSid();

    u8 sidNum;

    void sendAlive(void);

};

#endif /* _MB_SID_H */
