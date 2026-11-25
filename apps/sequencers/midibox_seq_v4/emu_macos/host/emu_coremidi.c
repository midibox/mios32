// MIDIbox SEQ emulator - the 4 USB MIDI ports of the core as CoreMIDI virtual ports "MBSEQ EMU Port 1..4"
// (MIOS32 USB MIDI packages <-> MIDI byte streams, incl. SysEx). The virtual panel and the flash tool connect to them.
#include <mios32.h>
#include <CoreMIDI/CoreMIDI.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "emu.h"
#include <unistd.h>
typedef unsigned long long u64_t;

#define NUM_PORTS 4
static MIDIClientRef client;
static MIDIEndpointRef src[NUM_PORTS], dst[NUM_PORTS];

// receive ring buffer (filled by the CoreMIDI thread, read by MIOS32_MIDI_Receive_Handler under the CPU lock)
#define RX_SIZE 4096
static mios32_midi_package_t rx[RX_SIZE];
static volatile u32 rx_head, rx_tail;
static pthread_mutex_t rx_lock = PTHREAD_MUTEX_INITIALIZER;
static void rx_put(mios32_midi_package_t p)
{
  pthread_mutex_lock(&rx_lock);
  u32 n = (rx_head + 1) % RX_SIZE;
  if( n != rx_tail ) { rx[rx_head] = p; rx_head = n; }
  pthread_mutex_unlock(&rx_lock);
}

// byte stream -> USB MIDI packages (per port parser state)
typedef struct { u8 status; u8 buf[3]; u8 n; u8 need; u8 sysex; } parser_t;
static parser_t ps[NUM_PORTS];
static u8 data_len(u8 st)
{
  if( st < 0xf0 ) { u8 h = st & 0xf0; return (h == 0xc0 || h == 0xd0) ? 1 : 2; }
  switch( st ) { case 0xf1: case 0xf3: return 1; case 0xf2: return 2; default: return 0; }
}
volatile int emu_activity = 0;     // a key/encoder came in: counts as front panel activity (screensaver)
static void emit(u8 cable, u8 cin, u8 b0, u8 b1, u8 b2)
{
  if( cin == 0x9 || cin == 0xb ) emu_activity = 1;
  static int log = -1; if( log < 0 ) log = getenv("EMU_MIDILOG") != NULL;
  if( log && cin != 0x4 && cin < 0x8 + 8 && !(cin >= 4 && cin <= 7) ) { printf("[MIDI IN %d] %02x %02x %02x\n", cable+1, b0, b1, b2); fflush(stdout); }
  mios32_midi_package_t p; p.ALL = 0;
  p.cin = cin; p.cable = cable; p.evnt0 = b0; p.evnt1 = b1; p.evnt2 = b2;
  rx_put(p);
}
void EMU_MIDI_ParseByte(u8 cable, u8 b);
static void parse_byte(u8 cable, u8 b) { EMU_MIDI_ParseByte(cable, b); }
void EMU_MIDI_ParseByte(u8 cable, u8 b)
{
  parser_t *s = &ps[cable];
  if( b >= 0xf8 ) { emit(cable, 0xf, b, 0, 0); return; }            // realtime
  if( b == 0xf0 ) { s->sysex = 1; s->n = 0; s->buf[s->n++] = b; return; }
  if( s->sysex ) {
    if( b == 0xf7 ) {
      s->buf[s->n++] = b;
      emit(cable, 4 + s->n, s->buf[0], s->n > 1 ? s->buf[1] : 0, s->n > 2 ? s->buf[2] : 0);   // CIN 5/6/7 = end with 1/2/3 bytes
      { static int log = -1; if( log < 0 ) log = getenv("EMU_MIDILOG") != NULL; if( log ) { printf("[MIDI IN %d] SysEx end\n", cable+1); fflush(stdout); } }
      s->sysex = 0; s->n = 0; return;
    }
    if( b & 0x80 ) { s->sysex = 0; s->n = 0; }                       // aborted sysex, handle as status below
    else {
      s->buf[s->n++] = b;
      if( s->n == 3 ) { emit(cable, 4, s->buf[0], s->buf[1], s->buf[2]); s->n = 0; }
      return;
    }
  }
  if( b & 0x80 ) {
    s->status = b; s->n = 0; s->need = data_len(b);
    if( s->need == 0 ) emit(cable, 5, b, 0, 0);                       // single byte system common
    return;
  }
  if( !s->status ) return;
  s->buf[s->n++] = b;
  if( s->n >= s->need ) {
    u8 st = s->status;
    u8 cin = (st < 0xf0) ? (st >> 4) : (s->need == 1 ? 2 : 3);
    emit(cable, cin, st, s->buf[0], s->need > 1 ? s->buf[1] : 0);
    s->n = 0;
    if( st >= 0xf0 ) s->status = 0;                                   // no running status for system common
  }
}
static void read_proc(const MIDIPacketList *pl, void *refCon, void *srcConn)
{
  u8 cable = (u8)(uintptr_t)refCon;
  const MIDIPacket *p = &pl->packet[0];
  for(UInt32 i=0; i<pl->numPackets; ++i) {
    for(UInt16 k=0; k<p->length; ++k) parse_byte(cable, p->data[k]);
    p = MIDIPacketNext(p);
  }
}

s32 EMU_MIDI_Init(void)
{
  if( MIDIClientCreate(CFSTR("MBSEQ EMU"), NULL, NULL, &client) != noErr ) {
    fprintf(stderr, "[EMU] CoreMIDI client failed\n"); return -1;
  }
  for(int i=0; i<NUM_PORTS; ++i) {
    char name[64]; snprintf(name, sizeof(name), "%s %d", EMU_PORT_NAME, i+1);
    CFStringRef n = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
    MIDISourceCreate(client, n, &src[i]);                              // emulator -> computer ("SEQ OUT")
    MIDIDestinationCreate(client, n, read_proc, (void *)(uintptr_t)i, &dst[i]);   // computer -> emulator ("SEQ IN")
    CFRelease(n);
  }
  extern s32 EMU_SOCKET_Init(void);
  return EMU_SOCKET_Init();
}

// ---- MIOS32 USB MIDI API ----
s32 MIOS32_USB_Init(u32 mode) { return 0; }
s32 MIOS32_USB_IsInitialized(void) { return 1; }
s32 MIOS32_USB_ForceSingleUSB(void) { return 0; }
s32 MIOS32_USB_MIDI_Init(u32 mode) { return 0; }
s32 MIOS32_USB_MIDI_CheckAvailable(u8 cable) { return cable < NUM_PORTS ? 1 : 0; }
s32 MIOS32_USB_MIDI_Periodic_mS(void) { return 0; }
s32 MIOS32_USB_MIDI_PackageReceive(mios32_midi_package_t *package)
{
  s32 r = -1;
  pthread_mutex_lock(&rx_lock);
  if( rx_tail != rx_head ) { *package = rx[rx_tail]; rx_tail = (rx_tail + 1) % RX_SIZE; r = 0; }
  pthread_mutex_unlock(&rx_lock);
  return r;
}
// MIOS32 debug messages (SysEx F0 00 00 7E 32 <dev> 0D 40 <text> F7) are also printed on the emulator console
static u8 dbg_buf[NUM_PORTS][1024]; static u16 dbg_len[NUM_PORTS];
static void debug_tap(u8 cable, const u8 *b, u8 len)
{
  for(int i=0; i<len; ++i) {
    u8 c = b[i];
    if( c == 0xf0 ) dbg_len[cable] = 0;
    if( dbg_len[cable] < sizeof(dbg_buf[0]) ) dbg_buf[cable][dbg_len[cable]++] = c;
    if( c == 0xf7 ) {
      u8 *d = dbg_buf[cable]; u16 n = dbg_len[cable];
      if( n > 9 && d[1] == 0 && d[2] == 0 && d[3] == 0x7e && d[4] == 0x32 && d[6] == 0x0d ) {
        printf("[SEQ] %.*s\n", (int)(n - 9 - (d[n-2] == 0x0a)), (const char *)&d[8]);
        fflush(stdout);
      }
      dbg_len[cable] = 0;
    }
  }
}

// send one complete MIDI message to the virtual source (SysEx is collected first: CoreMIDI receivers get whole messages)
volatile u32 emu_tx_msgs[NUM_PORTS], emu_tx_err[NUM_PORTS];
volatile u32 emu_tx_bad[NUM_PORTS];
// outgoing messages are collected per port and handed to CoreMIDI once per ms (EMU_MIDI_Flush, called from the 1 ms
// tick) - one MIDIReceived per message floods the MIDI server and it stops delivering
#define TXL_SIZE 4096     // larger packet lists per MIDIReceived make the MIDI server stop delivering
static Byte txl_buf[NUM_PORTS][TXL_SIZE];
static MIDIPacket *txl_cur[NUM_PORTS];
static u32 txl_n[NUM_PORTS];
void EMU_MIDI_Flush(void)
{
  for(int c=0; c<NUM_PORTS; ++c) {
    if( txl_n[c] ) {
      if( MIDIReceived(src[c], (MIDIPacketList *)txl_buf[c]) != noErr ) ++emu_tx_err[c];
      txl_n[c] = 0;
    }
  }
}
static void send_bytes(u8 cable, const u8 *b, u32 len)
{
  ++emu_tx_msgs[cable];
  { extern int EMU_SOCKET_Connected(void); extern void EMU_SOCKET_Send(u8 cable, const u8 *b, u32 len);
    EMU_SOCKET_Send(cable, b, len);
    if( EMU_SOCKET_Connected() && len > 8 && b[0] == 0xf0 && b[3] == 0x7e && b[4] == 0x4d && b[6] == 0x09 )
      return;                                                     // MBSEQ remote (screens, LEDs): only to the panel link
  }
  MIDIPacketList *pl = (MIDIPacketList *)txl_buf[cable];
  for(int attempt=0; attempt<2; ++attempt) {
    if( txl_n[cable] == 0 ) txl_cur[cable] = MIDIPacketListInit(pl);
    MIDIPacket *mp = MIDIPacketListAdd(pl, TXL_SIZE, txl_cur[cable], 0, len, b);
    if( mp ) { txl_cur[cable] = mp; ++txl_n[cable]; return; }
    EMU_MIDI_Flush();                                                // list full: send it now and retry
  }
  ++emu_tx_err[cable];
}
static u8 sx_buf[NUM_PORTS][16384];
static u32 sx_len[NUM_PORTS];

// USB full speed MIDI on the core moves 16 packages (64 bytes) per 1 ms frame; the firmware relies on "buffer full"
// to pace itself. Without this limit CoreMIDI gets flooded and stops delivering to the receivers.
#define TX_PACKAGES_PER_MS 16
#include <time.h>
static u64_t tx_ms; static u32 tx_count;
static int tx_budget(void)
{
  struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
  u64_t ms = (u64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
  if( ms != tx_ms ) { tx_ms = ms; tx_count = 0; }
  if( tx_count >= TX_PACKAGES_PER_MS ) return 0;
  ++tx_count;
  return 1;
}

s32 MIOS32_USB_MIDI_PackageSend_NonBlocking(mios32_midi_package_t p)
{
  if( p.cable >= NUM_PORTS ) return -1;
  if( !tx_budget() ) return -2;                                    // TX buffer full: try again later
  static const u8 len_of_cin[16] = { 0,0,2,3,3,1,2,3, 3,3,3,3,2,2,3,1 };
  u8 len = len_of_cin[p.cin];
  if( !len ) return 0;
  u8 bytes[3] = { p.evnt0, p.evnt1, p.evnt2 };
  u8 c = p.cable;
  if( p.cin >= 4 && p.cin <= 7 && !(p.cin == 5 && bytes[0] != 0xf7 && sx_len[c] == 0 && bytes[0] != 0xf0) ) {
    debug_tap(c, bytes, len);
    for(int i=0; i<len; ++i) {
      if( bytes[i] == 0xf0 ) sx_len[c] = 0;                      // a new SysEx always starts over
      if( sx_len[c] < sizeof(sx_buf[0]) ) sx_buf[c][sx_len[c]++] = bytes[i];
      if( bytes[i] == 0xf7 ) {
        if( sx_buf[c][0] == 0xf0 ) send_bytes(c, sx_buf[c], sx_len[c]);
        sx_len[c] = 0;
      }
    }
    return 0;
  }
  send_bytes(c, bytes, len);
  return 0;
}
s32 MIOS32_USB_MIDI_PackageSend(mios32_midi_package_t p)
{
  s32 r;
  while( (r = MIOS32_USB_MIDI_PackageSend_NonBlocking(p)) == -2 )   // blocking: wait for the next "USB frame"
    usleep(200);
  return r;
}
