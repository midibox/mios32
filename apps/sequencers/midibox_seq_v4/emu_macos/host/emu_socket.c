// MIDIbox SEQ emulator - direct link for the virtual panel: TCP 127.0.0.1:8767, frames <port 0..3> <len hi> <len lo>
// <MIDI bytes>. Same as the 4 USB MIDI ports, but without CoreMIDI: the macOS MIDI server stops delivering from a virtual
// source after ~100 KB of large SysEx streams; a direct link avoids that (see README).
#include <mios32.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "emu.h"

#define EMU_SOCKET_PORT 8767
static volatile int client = -1;
static pthread_mutex_t tx_lock = PTHREAD_MUTEX_INITIALIZER;
extern void EMU_MIDI_ParseByte(u8 cable, u8 b);       // emu_coremidi.c: byte stream -> USB MIDI packages

int EMU_SOCKET_Connected(void) { return client >= 0; }

void EMU_SOCKET_Send(u8 cable, const u8 *b, u32 len)
{
  if( client < 0 ) return;
  u8 hdr[3] = { cable, (u8)(len >> 8), (u8)len };
  pthread_mutex_lock(&tx_lock);
  if( send(client, hdr, 3, 0) != 3 || send(client, b, len, 0) != (ssize_t)len ) { close(client); client = -1; }
  pthread_mutex_unlock(&tx_lock);
}

static int recv_all(int fd, u8 *b, int n)
{
  int got = 0;
  while( got < n ) { ssize_t r = recv(fd, b + got, n - got, 0); if( r <= 0 ) return -1; got += r; }
  return 0;
}

static void *server_thread(void *arg)
{
  int srv = socket(AF_INET, SOCK_STREAM, 0);
  int one = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  struct sockaddr_in a; memset(&a, 0, sizeof(a));
  a.sin_family = AF_INET; a.sin_port = htons(EMU_SOCKET_PORT); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if( bind(srv, (struct sockaddr *)&a, sizeof(a)) < 0 || listen(srv, 1) < 0 ) {
    fprintf(stderr, "[EMU] panel link: port %d busy\n", EMU_SOCKET_PORT); return NULL;
  }
  for(;;) {
    int c = accept(srv, NULL, NULL);
    if( c < 0 ) continue;
    setsockopt(c, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if( client >= 0 ) close(client);
    client = c;
    printf("[EMU] panel connected (127.0.0.1:%d)\n", EMU_SOCKET_PORT); fflush(stdout);
    u8 hdr[3], buf[65536];
    while( client == c ) {
      if( recv_all(c, hdr, 3) < 0 ) break;
      u32 len = ((u32)hdr[1] << 8) | hdr[2];
      if( recv_all(c, buf, len) < 0 ) break;
      if( hdr[0] < 4 ) for(u32 i=0; i<len; ++i) EMU_MIDI_ParseByte(hdr[0], buf[i]);
    }
    if( client == c ) { close(c); client = -1; }
    printf("[EMU] panel disconnected\n"); fflush(stdout);
  }
  return NULL;
}

s32 EMU_SOCKET_Init(void)
{
  pthread_t t; pthread_create(&t, NULL, server_thread, NULL); pthread_detach(t);
  return 0;
}
