//
//  OscPort.h
//  Copy from UDPEcho example!
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


@interface OscPort : NSObject<UDPEchoDelegate> {
   UDPEcho *_echo;
}

- (BOOL)runServerOnPort:(NSUInteger)port;
- (BOOL)runClientWithHost:(NSString *)host port:(NSUInteger)port;
- (void)sendPacket:(NSData *)data;

@property (nonatomic, retain, readwrite) UDPEcho *      echo;

@end
