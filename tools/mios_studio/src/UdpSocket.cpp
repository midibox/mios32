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


// Unix headers for socket connections - add option for Windows here
#ifdef WIN32
  #include <winsock2.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
#endif
#include "UdpSocket.h"
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
bool UdpSocket::connect(const String& remoteHost, const unsigned& _portNumberRead, const unsigned& _portNumberWrite)
{
    struct sockaddr_in hostAddressInfo;
    struct hostent *remoteHostInfo;

    if( oscServerSocket != -1 )
        return false;

    portNumberWrite = _portNumberWrite;

#ifdef WIN32
	WSADATA wsaData;
	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)!=0) {
		return false;
    }
#endif


    // make a socket
    if( (oscServerSocket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        oscServerSocket = -1;
#ifdef WIN32
		WSACleanup();
#endif
        return false;
    }

    // get IP address from name
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
    remoteHostInfo = gethostbyname((const char *)remoteHost);
#else
    remoteHostInfo = gethostbyname(remoteHost.toCString());
#endif

#ifdef WIN32
	// This looks to be required in windows as remoteHostInfo is null if the host is not found
	// and that will cause an exception in the memcpy() below
	if (remoteHostInfo == NULL) {
		WSACleanup();
		return false;
	}
#endif

	// fill address struct
    memcpy(&remoteAddress, remoteHostInfo->h_addr, remoteHostInfo->h_length);

    hostAddressInfo.sin_addr.s_addr=INADDR_ANY;
    hostAddressInfo.sin_port=htons(_portNumberRead);
    hostAddressInfo.sin_family=AF_INET;

    if( bind(oscServerSocket, (struct sockaddr*)&hostAddressInfo, sizeof(hostAddressInfo)) < 0 ) {
        disconnect();
#ifdef WIN32
		WSACleanup();
#endif
        return false;
    }

    // make it non-blocking
#ifdef WIN32
	u_long iMode = 1; 
	if (ioctlsocket(oscServerSocket, FIONBIO, &iMode)!=0) {
		WSACleanup();
		return false; // error setting socket status
	}
#else
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
#endif

    return true;
}

//==============================================================================
void UdpSocket::disconnect(void)
{
#ifdef WIN32
    closesocket(oscServerSocket);
	WSACleanup();
#else
    close(oscServerSocket);
#endif
    oscServerSocket = -1;
}

//==============================================================================
unsigned UdpSocket::write(unsigned char *datagram, unsigned len)
{
    if( oscServerSocket < 0 )
        return 0;

    struct sockaddr_in remoteAddressInfo;
    remoteAddressInfo.sin_addr.s_addr = remoteAddress;
    remoteAddressInfo.sin_port        = htons(portNumberWrite);
    remoteAddressInfo.sin_family      = AF_INET;

    const struct sockaddr* remoteAddressInfoPtr = (sockaddr *)&remoteAddressInfo; // the sendto function expects a sockaddr parameter...
    return sendto(oscServerSocket, (const char*)datagram, len, 0, remoteAddressInfoPtr, sizeof(remoteAddressInfo));
}


//==============================================================================
unsigned UdpSocket::read(unsigned char *datagram, unsigned maxLen)
{
    if( oscServerSocket < 0 )
        return 0;

    int len = recv(oscServerSocket,(char*) datagram, maxLen, 0);

    return (len < 0) ? 0 : len;
}
