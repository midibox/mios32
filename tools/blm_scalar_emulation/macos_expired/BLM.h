// $Id$
//
//  BLM.h
//  blm_scalar_emulation
//
//  Created by Thorsten Klose on 19.11.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface BLM : NSMatrix {
}


- (id)delegate;
- (void)setDelegate:(id)newDelegate;

- (void)setColumns:(NSInteger)columns rows:(NSInteger)rows;

- (void)sendButtonEvent:(NSInteger)column:(NSInteger)row:(NSInteger)depressed;

- (NSInteger)numberOfLEDColours;

- (NSInteger)LED_State:(NSInteger)column:(NSInteger)row;
- (void)setLED_State:(NSInteger)column:(NSInteger)row:(NSInteger)state;

@end
