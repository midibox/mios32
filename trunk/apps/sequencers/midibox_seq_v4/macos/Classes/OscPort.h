//
//  OscPort.h
//  Copy from UDPEcho example!
//
//  Created by Thorsten Klose on 02.06.10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "UDPEcho.h"


@interface OscPort : NSObject<UDPEchoDelegate> {
   UDPEcho *_echo;
}

- (BOOL)runServerOnPort:(NSUInteger)port;
- (BOOL)runClientWithHost:(NSString *)host port:(NSUInteger)port;
- (void)sendPacket:(NSData *)data;

@property (nonatomic, retain, readwrite) UDPEcho *      echo;

@end
