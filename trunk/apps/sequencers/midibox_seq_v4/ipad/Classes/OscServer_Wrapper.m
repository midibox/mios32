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


// 1: errors and important messages
// 2: more debug messages
#define DEBUG_VERBOSE_LEVEL 1


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
#if DEBUG_VERBOSE_LEVEL >= 1
        NSLog(@"[OscServer_Wrapper] invalid host name: %@", hostName);
#endif
    } else {
        oscSendPort = [[OscPort alloc] init];
        if( oscSendPort == nil ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            NSLog(@"[OscServer_Wrapper] failed to create oscSendPort!");
#endif
        } else {
            if( [oscSendPort runClientWithHost:hostName port:sendPort] != YES ) {
#if DEBUG_VERBOSE_LEVEL >= 1
                NSLog(@"[OscServer_Wrapper] failed to open oscSendPort!");
#endif
            }
        }
    }

    NSUInteger receivePort = 10000;
    oscReceivePort = [[OscPort alloc] init];
    if( oscReceivePort == nil ) {
#if DEBUG_VERBOSE_LEVEL >= 1
        NSLog(@"[OscServer_Wrapper] failed to create oscReceivePort!");
#endif
    } else {
        if( [oscReceivePort runServerOnPort:receivePort] != YES ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            NSLog(@"[OscServer_Wrapper] failed to open oscReceivePort!");
#endif
        }
    }
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
static s32 OSC_SERVER_Method_MIDI(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // only checking for MIDI event ('m') -- we could support more parameter types here!
  if( osc_args->arg_type[0] == 'm' ) {
    mios32_midi_port_t port = method_arg;
    mios32_midi_package_t p = MIOS32_OSC_GetMIDI(osc_args->arg_ptr[0]);
#if DEBUG_VERBOSE_LEVEL >= 2
    NSLog(@"[OscServer_Wrapper] received MIDI %02X %02X %02X\n", p.evnt0, p.evnt1, p.evnt2);
#endif

    // propagate to application
    MIOS32_MIDI_SendPackageToRxCallback(port, p);
    APP_MIDI_NotifyPackage(port, p);
  }

  return 0; // no error
}

static const mios32_osc_search_tree_t parse_root[] = {
  { "midi",  NULL, &OSC_SERVER_Method_MIDI, OSC0 },
  { "midi1", NULL, &OSC_SERVER_Method_MIDI, OSC0 },
  { "midi2", NULL, &OSC_SERVER_Method_MIDI, OSC1 },
  { "midi3", NULL, &OSC_SERVER_Method_MIDI, OSC2 },
  { "midi4", NULL, &OSC_SERVER_Method_MIDI, OSC3 },
  { NULL, NULL, NULL, 0 } // terminator
};

s32 OSC_SERVER_ReceivePacket(u8 *packet, u32 len)
{
#if DEBUG_VERBOSE_LEVEL >= 2
    NSLog(@"[OscServer_Wrapper] received packet of len %d", len);
#endif

    s32 status = MIOS32_OSC_ParsePacket((u8 *)packet, len, parse_root);
    if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
        NSLog(@"Invalid OSC packet, status %d\n", status);
#endif
    }	

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
