/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  MbSeqViewController.m
//  MbSeq
//
//  Created by Thorsten Klose on 15.04.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "MbSeqViewController.h"

#include <mios32.h>
#include <app_lcd.h>

@implementation MbSeqViewController


/*
// The designated initializer. Override to perform setup that is required before the view is loaded.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
    }
    return self;
}
*/

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];

    CLcd* cLcd0 = APP_LCD_GetComponentPtr(0);
    cLcd0->setLcdColumns(40);
    cLcd0->setLcdLines(2);
    [cLcdLeft setCLcd:cLcd0];

    CLcd* cLcd1 = APP_LCD_GetComponentPtr(1);
    cLcd1->setLcdColumns(40);
    cLcd1->setLcdLines(2);
    [cLcdRight setCLcd:cLcd1];
	MIOS32_LCD_Init(0);

    MIOS32_LCD_DeviceSet(0);
    MIOS32_LCD_Clear();
    MIOS32_LCD_DeviceSet(1);
    MIOS32_LCD_Clear();

	// install background task
	NSTimer *timer1 = [NSTimer timerWithTimeInterval:0.01 target:self selector:@selector(backgroundTask:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: timer1 forMode: NSDefaultRunLoopMode];
}


//////////////////////////////////////////////////////////////////////////////
// the background task
// (in real HW called whenever nothing else is to do...
//////////////////////////////////////////////////////////////////////////////
- (void)backgroundTask:(NSTimer *)aTimer
{
	// -> forward to application
	//APP_Background();
    MIOS32_LCD_DeviceSet(0);
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintFormattedString("Hello World (Left)!\n");

    MIOS32_LCD_DeviceSet(1);
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintFormattedString("Hello World (Right)!\n");


    [cLcdLeft periodicUpdate];
    [cLcdRight periodicUpdate];
}


// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return YES;
}

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];

	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}

@end
