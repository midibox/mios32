/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  MbSeqViewController.h
//  MbSeq
//
//  Created by Thorsten Klose on 15.04.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <CLcdView.h>
#import "SeqButton.h"
#import "SeqEncoder.h"

@interface MbSeqViewController : UIViewController {

IBOutlet CLcdView *cLcdLeft;
IBOutlet CLcdView *cLcdRight;

IBOutlet SeqButton *buttonGroup1;
IBOutlet SeqButton *buttonGroup2;
IBOutlet SeqButton *buttonGroup3;
IBOutlet SeqButton *buttonGroup4;

IBOutlet SeqButton *buttonTrack1;
IBOutlet SeqButton *buttonTrack2;
IBOutlet SeqButton *buttonTrack3;
IBOutlet SeqButton *buttonTrack4;

IBOutlet SeqButton *buttonTLayerA;
IBOutlet SeqButton *buttonTLayerB;
IBOutlet SeqButton *buttonTLayerC;

IBOutlet SeqButton *buttonPLayerA;
IBOutlet SeqButton *buttonPLayerB;
IBOutlet SeqButton *buttonPLayerC;

IBOutlet SeqButton *buttonGP1;
IBOutlet SeqButton *buttonGP2;
IBOutlet SeqButton *buttonGP3;
IBOutlet SeqButton *buttonGP4;
IBOutlet SeqButton *buttonGP5;
IBOutlet SeqButton *buttonGP6;
IBOutlet SeqButton *buttonGP7;
IBOutlet SeqButton *buttonGP8;
IBOutlet SeqButton *buttonGP9;
IBOutlet SeqButton *buttonGP10;
IBOutlet SeqButton *buttonGP11;
IBOutlet SeqButton *buttonGP12;
IBOutlet SeqButton *buttonGP13;
IBOutlet SeqButton *buttonGP14;
IBOutlet SeqButton *buttonGP15;
IBOutlet SeqButton *buttonGP16;

IBOutlet SeqButton *buttonEdit;
IBOutlet SeqButton *buttonMute;
IBOutlet SeqButton *buttonPattern;
IBOutlet SeqButton *buttonSong;
IBOutlet SeqButton *buttonCopy;
IBOutlet SeqButton *buttonPaste;
IBOutlet SeqButton *buttonClear;

IBOutlet SeqButton *buttonLeft;
IBOutlet SeqButton *buttonRight;
IBOutlet SeqButton *buttonUtility;
IBOutlet SeqButton *buttonF1;
IBOutlet SeqButton *buttonF2;
IBOutlet SeqButton *buttonF3;
IBOutlet SeqButton *buttonF4;

IBOutlet SeqButton *buttonStepView;
IBOutlet SeqButton *buttonSolo;
IBOutlet SeqButton *buttonFast;
IBOutlet SeqButton *buttonAll;
IBOutlet SeqButton *buttonMenu;
IBOutlet SeqButton *buttonSelect;
IBOutlet SeqButton *buttonExit;

IBOutlet SeqButton *buttonScrub;
IBOutlet SeqButton *buttonMetronome;
IBOutlet SeqButton *buttonStop;
IBOutlet SeqButton *buttonPause;
IBOutlet SeqButton *buttonPlay;
IBOutlet SeqButton *buttonRew;
IBOutlet SeqButton *buttonFwd;

IBOutlet SeqEncoder *encDataWheel;
IBOutlet SeqEncoder *encGP1;
IBOutlet SeqEncoder *encGP2;
IBOutlet SeqEncoder *encGP3;
IBOutlet SeqEncoder *encGP4;
IBOutlet SeqEncoder *encGP5;
IBOutlet SeqEncoder *encGP6;
IBOutlet SeqEncoder *encGP7;
IBOutlet SeqEncoder *encGP8;
IBOutlet SeqEncoder *encGP9;
IBOutlet SeqEncoder *encGP10;
IBOutlet SeqEncoder *encGP11;
IBOutlet SeqEncoder *encGP12;
IBOutlet SeqEncoder *encGP13;
IBOutlet SeqEncoder *encGP14;
IBOutlet SeqEncoder *encGP15;
IBOutlet SeqEncoder *encGP16;

}

@end

