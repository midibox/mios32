//
//  OscServer_Wrapper.h
//
//  Created by Thorsten Klose on 02.06.10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#if TARGET_OS_EMBEDDED || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif
#import "UDPEcho.h"


@interface OscServer_Wrapper : NSObject<UDPEchoDelegate> {
}

@end
