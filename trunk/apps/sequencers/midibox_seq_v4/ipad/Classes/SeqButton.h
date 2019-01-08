/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  SeqButton.h
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 30.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SeqButton : UIButton {
        unsigned ledState;
}

- (unsigned)getLedState;
- (void)setLedState:(unsigned)state;


@end
