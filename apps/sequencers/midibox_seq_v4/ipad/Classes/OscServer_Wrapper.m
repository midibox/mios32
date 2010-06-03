/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
//
//  OscServer_Wrapper.m
//
//  Created by Thorsten Klose on 02.06.10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "OscServer_Wrapper.h"
#import "OscPort.h"

#include <mios32.h>
#include <netdb.h>


@implementation OscServer_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;

static OscPort *oscSendPort = nil;
static OscPort *oscReceivePort = nil;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
    oscSendPort = nil;
    oscReceivePort = nil;

    // TODO: make this configurable
    NSUInteger sendPort = 10001;
    NSString *hostName;
    hostName = [NSString stringWithUTF8String:"192.168.1.101"];
    if( hostName == nil ) {
        NSLog(@"[OscServer_Wrapper] invalid host name: %@", hostName);
    } else {
        oscSendPort = [[OscPort alloc] init];
        if( oscSendPort == nil )
            NSLog(@"[OscServer_Wrapper] failed to create oscSendPort!");
        else {
            if( [oscSendPort runClientWithHost:hostName port:sendPort] != YES )
                NSLog(@"[OscServer_Wrapper] failed to open oscSendPort!");
        }
    }

    NSUInteger receivePort = 10000;
    oscReceivePort = [[OscPort alloc] init];
    if( oscReceivePort == nil )
        NSLog(@"[OscServer_Wrapper] failed to create oscReceivePort!");
    else
        if( [oscReceivePort runServerOnPort:receivePort] != YES )
                NSLog(@"[OscServer_Wrapper] failed to open oscReceivePort!");
}


- (void) dealloc
{
    [oscSendPort dealloc];
    [oscReceivePort dealloc];
    [super dealloc];
}


/////////////////////////////////////////////////////////////////////////////
// Called by oscReceivePort when new data has been received
// (callback implemented in OscPort.m)
// TODO: this is a quick hack - we should better implement a delegate interface?
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_ReceivePacket(u8 *packet, u32 len)
{
    NSLog(@"[OscServer_Wrapper] received packet of len %d", len);
    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called by OSC client to send an UDP datagram
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_SendPacket(u8 *packet, u32 len)
{
  if( oscSendPort == nil )
      return -1;

  // exit immediately if length == 0
  if( len == 0 )
    return 0;

  NSData *data = [NSData dataWithBytes:(void *)packet length:(NSUInteger)len];
  [oscSendPort sendPacket:data];
  [data dealloc];
  return 0; // no error
}

@end
