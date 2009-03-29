/*
 * Copyright (c) 2002, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: telnet.c,v 1.1.2.3 2003/10/06 11:20:45 adam Exp $
 *
 */

#include "uip.h"

#include "telnet.h"
#include "uip_task.h"
#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

#define FLAG_CLOSE 1
#define FLAG_ABORT 2
/*-----------------------------------------------------------------------------------*/
unsigned char
telnet_send(struct telnet_state *s, char *text, u16_t len)
{
  if(s->text != NULL) {
    return 1;
  }
  s->text = text;
  s->textlen = len;
  s->sentlen = 0;
  return 0;
}
/*-----------------------------------------------------------------------------------*/
unsigned char
telnet_close(struct telnet_state *s)
{
  s->flags = FLAG_CLOSE;
  if(s->text != NULL) {
    return 1;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
unsigned char
telnet_abort(struct telnet_state *s)
{
  s->flags = FLAG_ABORT;
  if(s->text != NULL) {
    return 1;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
static void
acked(struct telnet_state *s)
{
  s->textlen -= s->sentlen;
  if(s->textlen == 0) {
    s->text = NULL;
    telnet_sent(s);
  } else {
    s->text += s->sentlen;
  }
  s->sentlen = 0;
}
/*-----------------------------------------------------------------------------------*/
static void
senddata(struct telnet_state *s)
{
  if(s->text == NULL) {
    uip_send(s->text, 0);
    return;
  }
  if(s->textlen > uip_mss()) {
    s->sentlen = uip_mss();
  } else {
    s->sentlen = s->textlen;
  }
  uip_send(s->text, s->sentlen);
}
/*-----------------------------------------------------------------------------------*/
void telnet_app(void)
{
  struct telnet_state *s = (struct telnet_state *)&(uip_conn->appstate);

  if(uip_connected()) {
    s->flags = 0;
    telnet_connected(s);
    senddata(s);
    return;
  }

  if(uip_closed()) {
    telnet_closed(s);
  }

  if(uip_aborted()) {
    telnet_aborted(s);
  }
  if(uip_timedout()) {
    telnet_timedout(s);
  }


  if(s->flags & FLAG_CLOSE) {
    uip_close();
    return;
  }
  if(s->flags & FLAG_ABORT) {
    uip_abort();
    return;
  }
  if(uip_acked()) {
    acked(s);
  }
  if(uip_newdata()) {
    telnet_newdata(s, (char *)uip_appdata, uip_datalen());
  }
  if(uip_rexmit() ||
     uip_newdata() ||
     uip_acked()) {
    senddata(s);
  } else if(uip_poll()) {
    senddata(s);
  }
}

/*-----------------------------------------------------------------------------------*/



/////////////////////////////////////////////////////////////////////////////
// Telnet client handlers
/////////////////////////////////////////////////////////////////////////////

void telnet_connected(struct telnet_state *s) {

#if TN_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[telnet] Connected!");
#endif

}

void telnet_closed(struct telnet_state *s) {

#if TN_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[telnet] Closed.");
#endif

}

void telnet_sent(struct telnet_state *s) {

#if TN_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[telnet] Data sent.");
#endif

}

void telnet_aborted(struct telnet_state *s) {

#if TN_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[telnet] ABORT!");
#endif

}

void telnet_timedout(struct telnet_state *s) {

#if TN_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[telnet] TIME OUT!");
#endif

}



/////////////////////////////////////////////////////////////////////////////
// Connect to server
// Call this from your app
// IP address and port to connect to as arguments
// in your app, save what this returns - you'll need it to send data
/////////////////////////////////////////////////////////////////////////////

struct telnet_state *telnet_connect(unsigned char ip0, unsigned char ip1,
                    unsigned char ip2, unsigned char ip3,
                    unsigned char port) {

    struct telnet_state * returnstruct;
    // take over exclusive access to UIP functions
    MUTEX_UIP_TAKE;

    uip_ipaddr_t ipaddr;
    uip_ipaddr(&ipaddr,ip0,ip1,ip2,ip3);
    returnstruct = &((uip_connect(&ipaddr, HTONS(port)))->appstate);

    // release exclusive access to UIP functions
    MUTEX_UIP_GIVE;

    return returnstruct;

}



/////////////////////////////////////////////////////////////////////////////
// Send string to server
// Pointer to this telnet state ( saved from telnet_connect() )
//  and string as arguments
/////////////////////////////////////////////////////////////////////////////

void telnet_sendstring(struct telnet_state *s, char *sendstring) {

    // take over exclusive access to UIP functions
    MUTEX_UIP_TAKE;

    char* telnetsendstring = sendstring;
    u16_t telnetstringlen;

    telnetstringlen = strlen(telnetsendstring);

    telnet_send(s, telnetsendstring, telnetstringlen);

    // release exclusive access to UIP functions
    MUTEX_UIP_GIVE;

}



/////////////////////////////////////////////////////////////////////////////
// Data received from server
// uIP will call this when it receives data
// you should call your handler from here, just after the debug message
/////////////////////////////////////////////////////////////////////////////

void telnet_newdata(struct telnet_state *s, char *data, u16_t len) {
    
#if TN_DEBUG_VERBOSE_LEVEL >= 1
    // echo the data out to the debug function for testing
    DEBUG_MSG("[telnet] New Data %s", data);
#endif
    
}



/////////////////////////////////////////////////////////////////////////////
// Abort connection
// Pointer to this telnet state ( saved from telnet_connect() ) as argument
/////////////////////////////////////////////////////////////////////////////

void telnet_forceabort(struct telnet_state *s) {
    // take over exclusive access to UIP functions
    MUTEX_UIP_TAKE;
    
    telnet_abort(s);
    
    // release exclusive access to UIP functions
    MUTEX_UIP_GIVE;

}



/////////////////////////////////////////////////////////////////////////////
// Disconnect from server
// Pointer to this telnet state ( saved from telnet_connect() ) as argument
/////////////////////////////////////////////////////////////////////////////

void telnet_disconnect(struct telnet_state *s) {
    // take over exclusive access to UIP functions
    MUTEX_UIP_TAKE;
    
    telnet_close(s);
    
    // release exclusive access to UIP functions
    MUTEX_UIP_GIVE;

}

