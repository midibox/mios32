// MIDIbox SEQ emulator - "OS": one global lock = one CPU, 1 ms task loops (as programming_models/traditional/main.c and
// apps/sequencers/midibox_seq_v4/mios32/tasks.c), hardware timers as threads. Everything that touches SEQ state runs
// while holding the CPU lock, so the firmware sees the same "one thing at a time" behaviour as on the STM32.
#include <mios32.h>
#include <app.h>
#include <pthread.h>
#include <CoreFoundation/CoreFoundation.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "emu.h"
#include "seq_midi_sysex.h"

static pthread_mutex_t cpu;
static volatile int running = 1;

static __thread unsigned slept_us;   // time this thread spent in EMU_CpuSleepUs (not holding the CPU)
static int cpu_depth;     // recursion depth of the owner (only touched while holding the lock)
void EMU_CpuLock(void)   { pthread_mutex_lock(&cpu); ++cpu_depth; }
void EMU_CpuUnlock(void) { --cpu_depth; pthread_mutex_unlock(&cpu); }
// Blocking waits (e.g. "USB buffer full") must not keep the one CPU: release it completely for a moment, so the BPM timer
// and the other loops keep running - on the STM32 their interrupts preempt such a wait.
void EMU_CpuSleepUs(unsigned us)
{
  int depth = cpu_depth;
  for(int i=0; i<depth; ++i) EMU_CpuUnlock();
  struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
  usleep(us);
  for(int i=0; i<depth; ++i) EMU_CpuLock();
  clock_gettime(CLOCK_MONOTONIC, &b); slept_us += (b.tv_sec - a.tv_sec) * 1000000L + (b.tv_nsec - a.tv_nsec) / 1000;
}
void EMU_CriticalEnter(void) { EMU_CpuLock(); }
void EMU_CriticalExit(void)  { EMU_CpuUnlock(); }
void EMU_Yield(void) {}
s32 MIOS32_IRQ_Disable(void) { EMU_CpuLock(); return 0; }
s32 MIOS32_IRQ_Enable(void)  { EMU_CpuUnlock(); return 0; }
s32 MIOS32_IRQ_Install(u8 IRQn, u8 priority) { return 0; }
s32 MIOS32_IRQ_DeInstall(u8 IRQn) { return 0; }

// SEQ semaphores (core/tasks.h, MIOS32_FAMILY_EMULATION): all code already runs under the CPU lock -> nothing to do
void TASKS_SDCardSemaphoreTake(void) {}  void TASKS_SDCardSemaphoreGive(void) {}
void TASKS_MIDIINSemaphoreTake(void) {}  void TASKS_MIDIINSemaphoreGive(void) {}
void TASKS_MIDIOUTSemaphoreTake(void) {} void TASKS_MIDIOUTSemaphoreGive(void) {}
void TASKS_LCDSemaphoreTake(void) {}     void TASKS_LCDSemaphoreGive(void) {}
void TASKS_MUTEX_MIDIOUT_Take(void) {}   void TASKS_MUTEX_MIDIOUT_Give(void) {}

static void ts_add_us(struct timespec *t, long us)
{
  t->tv_nsec += us * 1000L;
  while( t->tv_nsec >= 1000000000L ) { t->tv_nsec -= 1000000000L; ++t->tv_sec; }
}
static void sleep_until(const struct timespec *t)
{
  struct timespec now, d;
  clock_gettime(CLOCK_MONOTONIC, &now);
  d.tv_sec = t->tv_sec - now.tv_sec; d.tv_nsec = t->tv_nsec - now.tv_nsec;
  if( d.tv_nsec < 0 ) { d.tv_nsec += 1000000000L; --d.tv_sec; }
  if( d.tv_sec >= 0 ) nanosleep(&d, NULL);
}

// a periodic loop: fn() every period_us (catching up at most 5 periods, like the "xLastExecutionTime" logic)
typedef struct { void (*fn)(void); volatile u32 period_us; const char *name; volatile u32 calls; volatile u32 max_us; pthread_t thread; } loop_t;
static loop_t *all_loops[16]; static int num_loops;
static void *loop_thread(void *arg)
{
  loop_t *l = (loop_t *)arg;
  struct timespec next;
  clock_gettime(CLOCK_MONOTONIC, &next);
  while( running ) {
    ts_add_us(&next, l->period_us);
    sleep_until(&next);
    struct timespec now; clock_gettime(CLOCK_MONOTONIC, &now);
    if( (now.tv_sec - next.tv_sec) * 1000000L + (now.tv_nsec - next.tv_nsec) / 1000 > 5L * l->period_us )
      next = now;                                       // fell behind: don't burst
    if( !l->fn ) continue;
    EMU_CpuLock();
    if( !running ) { EMU_CpuUnlock(); break; }           // shutting down: don't start another callback
    struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0); slept_us = 0;
    l->fn();
    clock_gettime(CLOCK_MONOTONIC, &t1);
    { u32 us = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_nsec - t0.tv_nsec) / 1000; us = (us > slept_us) ? us - slept_us : 0; if( us > l->max_us ) l->max_us = us; }
    ++l->calls;
    EMU_CpuUnlock();
  }
  return NULL;
}
static loop_t *start_loop(void (*fn)(void), u32 period_us, const char *name)
{
  loop_t *l = calloc(1, sizeof(loop_t));
  l->fn = fn; l->period_us = period_us; l->name = name;
  if( num_loops < 16 ) all_loops[num_loops++] = l;
  pthread_create(&l->thread, NULL, loop_thread, l);
  return l;
}

// ---- MIOS32 timers (SEQ_BPM uses timer 0 for the sequencer clock) ----
static loop_t *timers[4];
s32 MIOS32_TIMER_Init(u8 timer, u32 period, void (*_irq_handler)(void), u8 irq_priority)
{
  if( timer >= 4 ) return -1;
  if( timers[timer] ) { timers[timer]->fn = _irq_handler; timers[timer]->period_us = period ? period : 1000; return 0; }
  timers[timer] = start_loop(_irq_handler, period ? period : 1000, "timer");
  return 0;
}
s32 MIOS32_TIMER_ReInit(u8 timer, u32 period)
{
  if( timer >= 4 || !timers[timer] ) return -1;
  timers[timer]->period_us = period ? period : 1000;
  return 0;
}
s32 MIOS32_TIMER_DeInit(u8 timer) { if( timer < 4 && timers[timer] ) timers[timer]->fn = NULL; return 0; }

// ---- programming model (traditional/main.c) ----
__attribute__((weak)) void APP_Tick(void) {}
__attribute__((weak)) void APP_MIDI_Tick(void) {}
static void SRIO_ServiceFinish(void)
{
  MIOS32_ENC_UpdateStates();
  APP_SRIO_ServiceFinish();
}
static void tick_hook(void)                  // vApplicationTickHook: timestamp + SRIO scan every ms
{
  MIOS32_TIMESTAMP_Inc();
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(SRIO_ServiceFinish);
  EMU_MIDI_Flush();
}
static void task_hooks(void)
{
  MIOS32_DIN_Handler(APP_DIN_NotifyToggle);
  MIOS32_ENC_Handler(APP_ENC_NotifyChange);
  MIOS32_COM_Receive_Handler();
  APP_Tick();
}
static void task_midi_hooks(void)
{
  MIOS32_MIDI_Periodic_mS();
  MIOS32_MIDI_Receive_Handler(APP_MIDI_NotifyPackage);
  APP_MIDI_Tick();
}

// ---- SEQ tasks (apps/sequencers/midibox_seq_v4/mios32/tasks.c) ----
extern void SEQ_TASK_MIDI(void);
extern void SEQ_TASK_Period1mS(void);
extern void SEQ_TASK_Period1mS_LowPrio(void);
extern void SEQ_TASK_Period1S(void);
static void task_lowprio(void)
{
  static u16 ms_ctr = 0;
  SEQ_TASK_Period1mS_LowPrio();
  if( ++ms_ctr >= 1000 ) { ms_ctr = 0; SEQ_TASK_Period1S(); }
}
s32 TASKS_Init(u32 mode)
{
  start_loop(SEQ_TASK_MIDI, 1000, "MIDI");
  start_loop(SEQ_TASK_Period1mS, 1000, "Period1mS");
  start_loop(task_lowprio, 1000, "Period1mS_LP");
  return 0;
}

static void *idle_thread(void *arg)
{
  int idle_ms = 0;
  while( running ) {                         // idle task: APP_Background
    if( getenv("EMU_STATS") && ++idle_ms % 5000 == 0 ) {
      for(int i=0; i<num_loops; ++i) { printf("[EMU] %-14s %u calls/5s, longest %u us\n", all_loops[i]->name, all_loops[i]->calls, all_loops[i]->max_us); all_loops[i]->calls = 0; all_loops[i]->max_us = 0; }
      {
        { extern volatile u32 emu_tx_msgs[4], emu_tx_err[4]; printf("[EMU] tx msgs %u %u %u %u errors %u %u %u %u\n", emu_tx_msgs[0], emu_tx_msgs[1], emu_tx_msgs[2], emu_tx_msgs[3], emu_tx_err[0], emu_tx_err[1], emu_tx_err[2], emu_tx_err[3]); }
        printf("[EMU] remote mode %d active_mode %d port 0x%02x active_port 0x%02x\n", seq_midi_sysex_remote_mode, seq_midi_sysex_remote_active_mode, seq_midi_sysex_remote_port, seq_midi_sysex_remote_active_port); }
      fflush(stdout);
    }
    EMU_CpuLock();
    if( !running ) { EMU_CpuUnlock(); break; }
    { extern volatile int emu_activity; extern s32 SEQ_LCD_LOGO_ScreenSaver_Disable(void);
      if( emu_activity ) { emu_activity = 0; SEQ_LCD_LOGO_ScreenSaver_Disable(); } }
    APP_Background();
    EMU_CpuUnlock();
    usleep(1000);
  }
  return NULL;
}

static void on_signal(int s) { running = 0; }

int main(int argc, char **argv)
{
  pthread_mutexattr_t a;
  pthread_mutexattr_init(&a);
  pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&cpu, &a);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  if( EMU_Setup(argc, argv) < 0 )
    return 1;

  if( getenv("EMU_FSTEST") ) {                 // FatFs write test on the SD card image
    #include "ff.h"
    static FATFS fs; static FIL f; UINT bw;
    MIOS32_SDCARD_Init(0);
    printf("[FSTEST] f_mount %d\n", f_mount(0, &fs));
    printf("[FSTEST] f_open  %d\n", f_open(&f, "0:/TEST.TXT", FA_CREATE_ALWAYS | FA_WRITE));
    printf("[FSTEST] f_write %d (%u)\n", f_write(&f, "hello\n", 6, &bw), bw);
    printf("[FSTEST] f_close %d\n", f_close(&f));
    printf("[FSTEST] f_mkdir %d\n", f_mkdir("0:/TESTDIR"));
    { static DIR d; printf("[FSTEST] f_opendir /SESSIONS/DEFAULT %d\n", f_opendir(&d, "/SESSIONS/DEFAULT")); }
    printf("[FSTEST] f_open /SESSIONS/DEFAULT/MBSEQ_C.V4 %d\n", f_open(&f, "/SESSIONS/DEFAULT/MBSEQ_C.V4", FA_CREATE_ALWAYS | FA_WRITE)); f_close(&f);
    printf("[FSTEST] f_open /TESTDIR/X.V4 %d\n", f_open(&f, "/TESTDIR/X.V4", FA_CREATE_ALWAYS | FA_WRITE)); f_close(&f);
    { DWORD fre; FATFS *pfs; printf("[FSTEST] f_getfree %d free clusters %u fs_type %d csize %d\n", f_getfree("0:", &fre, &pfs), (unsigned)fre, pfs ? pfs->fs_type : -1, pfs ? pfs->csize : -1); }
    return 0;
  }
  EMU_CpuLock();
  MIOS32_SYS_Init(0);
  MIOS32_DELAY_Init(0);
  MIOS32_TIMESTAMP_Init(0);
  MIOS32_BOARD_Init(0);
  MIOS32_SPI_Init(0);
  MIOS32_SRIO_Init(0);
  MIOS32_DIN_Init(0);
  MIOS32_DOUT_Init(0);
  MIOS32_ENC_Init(0);
  MIOS32_MIDI_Init(0);
  MIOS32_USB_Init(0);
  MIOS32_COM_Init(0);
  MIOS32_LCD_Init(0);
  APP_Init();
  EMU_CpuUnlock();

  start_loop(tick_hook, 1000, "tick");
  start_loop(task_hooks, 1000, "hooks");
  start_loop(task_midi_hooks, 1000, "midi_hooks");
  printf("[EMU] MIDIbox SEQ V4+ running - MIDI ports \"%s 1..4\". Ctrl-C to stop.\n", EMU_PORT_NAME);
  fflush(stdout);

  // idle task (APP_Background) in its own thread; the main thread runs the CoreFoundation run loop that CoreMIDI
  // needs for its notifications (new listeners, setup changes)
  pthread_t idle; pthread_create(&idle, NULL, idle_thread, NULL);
  while( running )
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.2, false);
  printf("\n[EMU] stopped\n");
  // wait until no callback and no worker is running any more, only then close the SD card image
  pthread_join(idle, NULL);
  for(int i=0; i<num_loops; ++i) pthread_join(all_loops[i]->thread, NULL);
  EMU_CpuLock();
  EMU_Shutdown();
  EMU_CpuUnlock();
  return 0;
}
