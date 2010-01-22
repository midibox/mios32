/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$

#ifndef _MB_SID_ENVIRONMENT_H
#define _MB_SID_ENVIRONMENT_H

#include <mios32.h>
#include "MbSid.h"

class MbSidEnvironment
{
public:
    // Constructor
    MbSidEnvironment();

    // Destructor
    ~MbSidEnvironment();

    MbSid mbSid[4];

    void test();
};

#endif /* _MB_SID_ENVIRONMENT_H */
