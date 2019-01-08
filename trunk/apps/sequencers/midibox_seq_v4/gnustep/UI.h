// $Id: UI.h 159 2008-12-06 01:33:47Z tk $
//
//  UI.h
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "CLCDView.h"

@interface UI : NSObject {
IBOutlet NSColorWell *LEDBeat;

IBOutlet NSColorWell *LED1;
IBOutlet NSColorWell *LED2;
IBOutlet NSColorWell *LED3;
IBOutlet NSColorWell *LED4;
IBOutlet NSColorWell *LED5;
IBOutlet NSColorWell *LED6;
IBOutlet NSColorWell *LED7;
IBOutlet NSColorWell *LED8;
IBOutlet NSColorWell *LED9;
IBOutlet NSColorWell *LED10;
IBOutlet NSColorWell *LED11;
IBOutlet NSColorWell *LED12;
IBOutlet NSColorWell *LED13;
IBOutlet NSColorWell *LED14;
IBOutlet NSColorWell *LED15;
IBOutlet NSColorWell *LED16;

IBOutlet NSButton *buttonGroup1;
IBOutlet NSButton *buttonGroup2;
IBOutlet NSButton *buttonGroup3;
IBOutlet NSButton *buttonGroup4;

IBOutlet NSButton *buttonTrack1;
IBOutlet NSButton *buttonTrack2;
IBOutlet NSButton *buttonTrack3;
IBOutlet NSButton *buttonTrack4;

IBOutlet NSButton *buttonTLayerA;
IBOutlet NSButton *buttonTLayerB;
IBOutlet NSButton *buttonTLayerC;

IBOutlet NSButton *buttonPLayerA;
IBOutlet NSButton *buttonPLayerB;
IBOutlet NSButton *buttonPLayerC;

IBOutlet NSButton *buttonGP1;
IBOutlet NSButton *buttonGP2;
IBOutlet NSButton *buttonGP3;
IBOutlet NSButton *buttonGP4;
IBOutlet NSButton *buttonGP5;
IBOutlet NSButton *buttonGP6;
IBOutlet NSButton *buttonGP7;
IBOutlet NSButton *buttonGP8;
IBOutlet NSButton *buttonGP9;
IBOutlet NSButton *buttonGP10;
IBOutlet NSButton *buttonGP11;
IBOutlet NSButton *buttonGP12;
IBOutlet NSButton *buttonGP13;
IBOutlet NSButton *buttonGP14;
IBOutlet NSButton *buttonGP15;
IBOutlet NSButton *buttonGP16;

IBOutlet NSButton *buttonEdit;
IBOutlet NSButton *buttonMute;
IBOutlet NSButton *buttonPattern;
IBOutlet NSButton *buttonSong;
IBOutlet NSButton *buttonCopy;
IBOutlet NSButton *buttonPaste;
IBOutlet NSButton *buttonClear;

IBOutlet NSButton *buttonLeft;
IBOutlet NSButton *buttonRight;
IBOutlet NSButton *buttonUtility;
IBOutlet NSButton *buttonF1;
IBOutlet NSButton *buttonF2;
IBOutlet NSButton *buttonF3;
IBOutlet NSButton *buttonF4;

IBOutlet NSButton *buttonStepView;
IBOutlet NSButton *buttonSolo;
IBOutlet NSButton *buttonFast;
IBOutlet NSButton *buttonAll;
IBOutlet NSButton *buttonMenu;
IBOutlet NSButton *buttonSelect;
IBOutlet NSButton *buttonExit;

IBOutlet NSButton *buttonScrub;
IBOutlet NSButton *buttonMetronome;
IBOutlet NSButton *buttonStop;
IBOutlet NSButton *buttonPause;
IBOutlet NSButton *buttonPlay;
IBOutlet NSButton *buttonRew;
IBOutlet NSButton *buttonFwd;

}

@end
