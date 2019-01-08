/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
//
//  OscPort.m
//  Copy from UDPEcho example!
//
//  Created by Thorsten Klose on 02.06.10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "OscPort.h"

#include <netdb.h>


// 1: errors and important messages
// 2: more debug messages
#define DEBUG_VERBOSE_LEVEL 1

@implementation OscPort

- (void)dealloc
{
    [self->_echo stop];
    [self->_echo release];
    [super dealloc];
}

@synthesize echo      = _echo;

//////////////////////////////////////////////////////////////////////////////
// OSC Port utility functions (from UDPEcho example)
//////////////////////////////////////////////////////////////////////////////
static NSString * DisplayAddressForAddress(NSData * address)
    // Returns a dotted decimal string for the specified address (a (struct sockaddr) 
    // within the address NSData).
{
    int         err;
    NSString *  result;
    char        hostStr[NI_MAXHOST];
    char        servStr[NI_MAXSERV];
    
    result = nil;
    
    if (address != nil) {
 
        // If it's a IPv4 address embedded in an IPv6 address, just bring it as an IPv4 
        // address.  Remember, this is about display, not functionality, and users don't 
        // want to see mapped addresses.
        
        if ([address length] >= sizeof(struct sockaddr_in6)) {
            const struct sockaddr_in6 * addr6Ptr;
            
            addr6Ptr = [address bytes];
            if (addr6Ptr->sin6_family == AF_INET6) {
                if ( IN6_IS_ADDR_V4MAPPED(&addr6Ptr->sin6_addr) || IN6_IS_ADDR_V4COMPAT(&addr6Ptr->sin6_addr) ) {
                    struct sockaddr_in  addr4;
                    
                    memset(&addr4, 0, sizeof(addr4));
                    addr4.sin_len         = sizeof(addr4);
                    addr4.sin_family      = AF_INET;
                    addr4.sin_port        = addr6Ptr->sin6_port;
                    addr4.sin_addr.s_addr = addr6Ptr->sin6_addr.__u6_addr.__u6_addr32[3];
                    address = [NSData dataWithBytes:&addr4 length:sizeof(addr4)];
                    assert(address != nil);
                }
            }
        }
        err = getnameinfo([address bytes], (socklen_t) [address length], hostStr, sizeof(hostStr), servStr, sizeof(servStr), NI_NUMERICHOST | NI_NUMERICSERV);
        if (err == 0) {
            result = [NSString stringWithFormat:@"%s:%s", hostStr, servStr];
            assert(result != nil);
        }
    }
 
    return result;
}
 
static NSString * DisplayStringFromData(NSData *data)
    // Returns a human readable string for the given data.
{
    NSMutableString *   result;
    NSUInteger          dataLength;
    NSUInteger          dataIndex;
    const uint8_t *     dataBytes;
 
    assert(data != nil);
    
    dataLength = [data length];
    dataBytes  = [data bytes];
 
    result = [NSMutableString stringWithCapacity:dataLength];
    assert(result != nil);
 
    [result appendString:@"\""];
    for (dataIndex = 0; dataIndex < dataLength; dataIndex++) {
        uint8_t     ch;
        
        ch = dataBytes[dataIndex];
        if (ch == 10) {
            [result appendString:@"\n"];
        } else if (ch == 13) {
            [result appendString:@"\r"];
        } else if (ch == '"') {
            [result appendString:@"\\\""];
        } else if (ch == '\\') {
            [result appendString:@"\\\\"];
        } else if ( (ch >= ' ') && (ch < 127) ) {
            [result appendFormat:@"%c", (int) ch];
        } else {
            [result appendFormat:@"\\x%02x", (unsigned int) ch];
        }
    }
    [result appendString:@"\""];
    
    return result;
}
 
static NSString * DisplayErrorFromError(NSError *error)
    // Given an NSError, returns a short error string that we can print, handling 
    // some special cases along the way.
{
    NSString *      result;
    NSNumber *      failureNum;
    int             failure;
    const char *    failureStr;
    
    assert(error != nil);
    
    result = nil;
    
    // Handle DNS errors as a special case.
    
    if ( [[error domain] isEqual:(NSString *)kCFErrorDomainCFNetwork] && ([error code] == kCFHostErrorUnknown) ) {
        failureNum = [[error userInfo] objectForKey:(id)kCFGetAddrInfoFailureKey];
        if ( [failureNum isKindOfClass:[NSNumber class]] ) {
            failure = [failureNum intValue];
            if (failure != 0) {
                failureStr = gai_strerror(failure);
                if (failureStr != NULL) {
                    result = [NSString stringWithUTF8String:failureStr];
                    assert(result != nil);
                }
            }
        }
    }
    
    // Otherwise try various properties of the error object.
    
    if (result == nil) {
        result = [error localizedFailureReason];
    }
    if (result == nil) {
        result = [error localizedDescription];
    }
    if (result == nil) {
        result = [error description];
    }
    assert(result != nil);
    return result;
}


//////////////////////////////////////////////////////////////////////////////
// OSC port access functions (from UDPEcho example)
//////////////////////////////////////////////////////////////////////////////
- (BOOL)runServerOnPort:(NSUInteger)port
    // One of two Objective-C 'mains' for this program.  This creates a UDPEcho 
    // object and runs it in server mode.
{
    assert(self.echo == nil);
    
    self.echo = [[[UDPEcho alloc] init] autorelease];
    assert(self.echo != nil);
    
    self.echo.delegate = self;
 
    [self.echo startServerOnPort:port];
    
    if( self.echo != nil ) {
        return YES;
    }
    
    return NO;
}
 
- (BOOL)runClientWithHost:(NSString *)host port:(NSUInteger)port
    // One of two Objective-C 'mains' for this program.  This creates a UDPEcho 
    // object in client mode, talking to the specified host and port, and then 
    // periodically sends packets via that object.
{
    assert(host != nil);
    assert( (port > 0) && (port < 65536) );
 
    assert(self.echo == nil);
 
    self.echo = [[[UDPEcho alloc] init] autorelease];
    assert(self.echo != nil);
    
    self.echo.delegate = self;
 
    [self.echo startConnectedToHostName:host port:port];
    
    return self.echo != nil ? YES : NO;
}

- (void)sendPacket:(NSData *)data;
    // Called by the client code to send a UDP packet.
{
    [self.echo sendData:data];
}

 
- (void)echo:(UDPEcho *)echo didReceiveData:(NSData *)data fromAddress:(NSData *)addr
    // This UDPEcho delegate method is called after successfully receiving data.
{
    assert(echo == self.echo);
    #pragma unused(echo)
    assert(data != nil);
    assert(addr != nil);
#if DEBUG_VERBOSE_LEVEL >= 2
    NSLog(@"[OscPort] received %@ from %@", DisplayStringFromData(data), DisplayAddressForAddress(addr));
#endif

    // TK: forward to OSC server
    // TODO: this is a quick hack - we should better implement a delegate interface?
    OSC_SERVER_ReceivePacket([data bytes], [data length]);
}
 
- (void)echo:(UDPEcho *)echo didReceiveError:(NSError *)error
    // This UDPEcho delegate method is called after a failure to receive data.
{
    assert(echo == self.echo);
    #pragma unused(echo)
    assert(error != nil);
#if DEBUG_VERBOSE_LEVEL >= 1
    NSLog(@"[OscPort] received error: %@", DisplayErrorFromError(error));
#endif
}
 
- (void)echo:(UDPEcho *)echo didSendData:(NSData *)data toAddress:(NSData *)addr
    // This UDPEcho delegate method is called after successfully sending data.
{
    assert(echo == self.echo);
    #pragma unused(echo)
    assert(data != nil);
    assert(addr != nil);
#if DEBUG_VERBOSE_LEVEL >= 2
    NSLog(@"[OscPort] sent %@ to   %@", DisplayStringFromData(data), DisplayAddressForAddress(addr));
#endif
}
 
- (void)echo:(UDPEcho *)echo didFailToSendData:(NSData *)data toAddress:(NSData *)addr error:(NSError *)error
    // This UDPEcho delegate method is called after a failure to send data.
{
    assert(echo == self.echo);
    #pragma unused(echo)
    assert(data != nil);
    assert(addr != nil);
    assert(error != nil);
#if DEBUG_VERBOSE_LEVEL >= 2
    NSLog(@"[OscPort] sending %@ to   %@, error: %@", DisplayStringFromData(data), DisplayAddressForAddress(addr), DisplayErrorFromError(error));
#endif
}
 
- (void)echo:(UDPEcho *)echo didStartWithAddress:(NSData *)address;
    // This UDPEcho delegate method is called after the object has successfully started up.
{
    assert(echo == self.echo);
    #pragma unused(echo)
    assert(address != nil);
    
#if DEBUG_VERBOSE_LEVEL >= 1
    if (self.echo.isServer) {
        NSLog(@"[OscPort] receiving on %@", DisplayAddressForAddress(address));
    } else {
        NSLog(@"[OscPort] sending to %@", DisplayAddressForAddress(address));
    }
#endif
}
 
- (void)echo:(UDPEcho *)echo didStopWithError:(NSError *)error
    // This UDPEcho delegate method is called  after the object stops spontaneously.
{
    assert(echo == self.echo);
    #pragma unused(echo)
    assert(error != nil);
#if DEBUG_VERBOSE_LEVEL >= 1
    NSLog(@"[OscPort] failed with error: %@", DisplayErrorFromError(error));
#endif
    self.echo = nil;
}

@end
