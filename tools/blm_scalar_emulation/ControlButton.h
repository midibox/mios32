// $Id$
//
//  ControlButton.h
//  blm_scalar_emulation
//
//  Created by Thorsten Klose on 19.11.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface ControlButton : NSButton {
	NSInteger LED_State;
	NSInteger buttonChn;
	NSInteger buttonCC;
}

- (id)delegate;
- (void)setDelegate:(id)newDelegate;

- (NSInteger)buttonChn;
- (void)setButtonChn:(NSInteger)chn;
- (NSInteger)buttonCC;
- (void)setButtonCC:(NSInteger)cc;

- (NSInteger)LED_State;
- (void)setLED_State:(NSInteger)state;

@end
