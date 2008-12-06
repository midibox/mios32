//
//  MIOS32_LCD_Wrapper.h
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "CLCDView.h"


@interface MIOS32_LCD_Wrapper : NSObject {

IBOutlet CLCDView *lcdView1;
IBOutlet CLCDView *lcdView2;

}

@end
