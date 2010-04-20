/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * UDP Socket
 *
 * We don't use the Juce based DatagramSocket since it behaves sometimes strange
 * (e.g. hangups) and since it seems that it isn't made for exchanging datagrams
 * without blocking.
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "UdpSocket.h"

// Unix headers for socket connections - add option for Windows here
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>


//==============================================================================
UdpSocket::UdpSocket()
    : oscServerSocket(-1)
{
}

UdpSocket::~UdpSocket()
{
    disconnect();
}


//==============================================================================
bool UdpSocket::connect(const String& remoteHost, const unsigned& _portNumber)
{
    struct sockaddr_in hostAddressInfo;
    struct hostent *remoteHostInfo;

    if( oscServerSocket != -1 )
        return false;

    portNumber = _portNumber;

    // make a socket
    if( (oscServerSocket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        oscServerSocket = -1;
        return false;
    }

    // get IP address from name
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
    remoteHostInfo = gethostbyname((const char *)remoteHost);
#else
    remoteHostInfo = gethostbyname(remoteHost.toCString());
#endif

    // fill address struct
    memcpy(&remoteAddress, remoteHostInfo->h_addr, remoteHostInfo->h_length);

    hostAddressInfo.sin_addr.s_addr=INADDR_ANY;
    hostAddressInfo.sin_port=htons(portNumber);
    hostAddressInfo.sin_family=AF_INET;

    if( bind(oscServerSocket, (struct sockaddr*)&hostAddressInfo, sizeof(hostAddressInfo)) < 0 ) {
        disconnect();
        return false;
    }

    // make it non-blocking
    int status;
    if( fcntl(oscServerSocket, F_GETFL) < 0 ) {
        disconnect();
        return false; // error while getting socket status
    }

    status |= O_NONBLOCK;

    if( fcntl(oscServerSocket, F_SETFL, status) < 0 ) {
        disconnect();
        return false; // error while setting socket status
    }

    return true;
}

//==============================================================================
void UdpSocket::disconnect(void)
{
    close(oscServerSocket);
    oscServerSocket = -1;
}

//==============================================================================
unsigned UdpSocket::write(unsigned char *datagram, unsigned len)
{
    if( oscServerSocket < 0 )
        return 0;

    struct sockaddr_in remoteAddressInfo;
    remoteAddressInfo.sin_addr.s_addr = remoteAddress;
    remoteAddressInfo.sin_port        = htons(portNumber);
    remoteAddressInfo.sin_family      = AF_INET;

    const struct sockaddr* remoteAddressInfoPtr = (sockaddr *)&remoteAddressInfo; // the sendto function expects a sockaddr parameter...
    return sendto(oscServerSocket, datagram, len, 0, remoteAddressInfoPtr, sizeof(remoteAddressInfo));
}


//==============================================================================
unsigned UdpSocket::read(unsigned char *datagram, unsigned maxLen)
{
    if( oscServerSocket < 0 )
        return 0;

    int len = recv(oscServerSocket, datagram, maxLen, 0);

    return (len < 0) ? 0 : len;
}
