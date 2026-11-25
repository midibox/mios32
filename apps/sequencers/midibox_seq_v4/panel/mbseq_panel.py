#!/usr/bin/env python3
"""
MIDIbox SEQ V4+ - virtual front panel (layout of Wilba's front panel) in the browser + HTTP API.
Works with a real SEQ over USB MIDI and with the macOS emulator (emu_macos/run.sh, option --emu).

  python3 mbseq_panel.py            -> real SEQ: start the server, open http://127.0.0.1:8765
  python3 mbseq_panel.py --no-browser
  python3 mbseq_panel.py --emu --http 8766   -> emulator (direct link 127.0.0.1:8767, no MIDI ports)

How it works (see seq_midi_sysex.c / seq_ui.c / seq_midi_in.c in the firmware):
  * LCD + LEDs : SysEx "remote server" on Port 1. Every second a REFRESH
                 (F0 00 00 7E 4D 00 09 01 F7); the SEQ sends LCD changes (09 02), the charset (09 03)
                 and the LED shift registers (09 04). On Wilba's panel the LEDs are an 8x8 matrix
                 (row select SR1, LEDs SR2), decoded here. A real SEQ needs RemotePort 0x10 in MBSEQ_GC.V4.
  * Encoders   : CC 15 (datawheel) / 16..31 (GP1..16), value 64+delta, channel 1, Port 1.
  * Buttons    : Wilba's buttons are a matrix (DIN pin >= 256) and can't be reached with the remote
                 protocol, so the "MIDI Remote Keyboard" function is used: hold note 96 on Port 2
                 (channel 1), then other notes = front panel buttons (SEQ_UI_REMOTE_MIDI_Keyboard).
  * SAVE ALL   : MIOS terminal command "store".

API (for scripts; everything that changes something is POST, requests from other web pages are refused):
  GET  /api/screen                 -> LCD as plain text (2 lines x 80)
  GET  /api/state                  -> JSON: lcd, leds, charset, screen
  GET  /api/buttons                -> list of button names
  POST /api/press?b=EDIT           -> tap a button; combination: b=SELECT+GP3 (the first one is held)
         &ms=120                      (optional: how long it is pressed)
  POST /api/down?b=FAST  /api/up?b=FAST   -> hold / release a button
  POST /api/enc?e=GP3&d=2          -> turn an encoder (e = DATAWHEEL or GP1..GP16, d = -63..63)
  POST /api/sync                   -> let the SEQ send the whole screen again (MENU tap)
  POST /api/store                  -> save the session (SAVE ALL)
  POST /api/fb?cmd=dir%20/SESSIONS -> MIOS file browser command (same protocol as MIOS Studio), returns the replies
  POST /api/upload?src=<dir>&name=<session> -> copy a local folder to /SESSIONS/<session> on the SD card
"""
import argparse
import base64
import json
import os
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

SEQ_HDR = [0x00, 0x00, 0x7E, 0x4D]      # MBSEQ SysEx header (without F0)
MIOS_HDR = [0x00, 0x00, 0x7E, 0x32]     # MIOS32 SysEx header (terminal)
REMOTE_KEY = 96                         # MidiRemoteKey in MBSEQ_GC.V4

# ---------------------------------------------------------------- Wilba hardware
# LED -> (matrix row 0..7 = M1..M8, bit)   (hwcfg/wilba/MBSEQ_HW.V4)
LEDS = {
    "TRACK1": (6, 2), "TRACK2": (6, 1), "TRACK3": (4, 2), "TRACK4": (4, 1),
    "PAR_A": (3, 2), "PAR_B": (3, 1), "PAR_C": (3, 0),
    "TRG_A": (1, 2), "TRG_B": (1, 1), "TRG_C": (1, 0),
    "GROUP1": (7, 3), "GROUP2": (7, 2), "GROUP3": (7, 1), "GROUP4": (7, 0),
    "EDIT": (4, 3), "MUTE": (5, 3), "PATTERN": (5, 2), "SONG": (6, 3),
    "SOLO": (5, 1), "FAST": (5, 0), "ALL": (6, 0), "STEP_VIEW": (2, 1),
    "PLAY": (0, 3), "STOP": (2, 3), "PAUSE": (1, 3), "REW": (2, 2), "FWD": (0, 2),
    "BEAT": (0, 1),
}
# GP LEDs: dual colour, 2 per row (seq_ui.c, dout_gp_mapping 1)
GP_ROW = [4, 4, 5, 5, 6, 6, 7, 7, 3, 3, 2, 2, 1, 1, 0, 0]

# button -> note of the MIDI Remote Keyboard function (SEQ_UI_REMOTE_MIDI_Keyboard)
_GP_NOTES = [0x24, 0x26, 0x28, 0x29, 0x2B, 0x2D, 0x2F, 0x30,
             0x32, 0x34, 0x35, 0x37, 0x39, 0x3B, 0x3C, 0x3E]
BUTTON_NOTES = {f"GP{i + 1}": n for i, n in enumerate(_GP_NOTES)}
BUTTON_NOTES.update({
    "TRACK1": 0x25, "TRACK2": 0x27, "TRACK3": 0x2A, "TRACK4": 0x2C,
    "PAR_A": 0x2E, "PAR_B": 0x31, "PAR_C": 0x33,
    "TRG_A": 0x36, "TRG_B": 0x38, "TRG_C": 0x3A,
    "GROUP1": 0x3D, "GROUP2": 0x3F, "GROUP3": 0x42, "GROUP4": 0x44,
    "STEP_VIEW": 0x41, "LEFT": 0x45, "RIGHT": 0x47,
    "EDIT": 0x48, "SOLO": 0x49, "MUTE": 0x4A, "ALL": 0x4B, "PATTERN": 0x4C,
    "SONG": 0x4D, "FAST": 0x4E, "METRONOME": 0x4F, "TRACK_SEL": 0x52,
    "STOP": 0x53, "PLAY": 0x54, "PAUSE": 0x55, "REW": 0x56, "FWD": 0x57,
    "UTILITY": 0x58, "MENU": 0x5B, "SELECT": 0x5C, "EXIT": 0x5D,
    "DOWN": 0x5E, "UP": 0x5F, "PAR_SEL": 0x51,
})
SPECIAL_BUTTONS = {"SAVE_ALL"}                       # via terminal
UNAVAILABLE = {"SCRUB"}                              # not reachable remotely
ENCODERS = {"DATAWHEEL": 0, **{f"GP{i + 1}": i + 1 for i in range(16)}}

# text version of the custom characters 0..7 per charset (order of seq_lcd.h)
CHARSET_TEXT = {
    1: "<>↔     ",              # Menu: arrows
    2: "▁▂▃▄▅▆▇█",  # VBars
    3: " ▏▎▍·>  ",                       # HBars
    4: "·●○◉    ",                       # drums big
    5: " ··:    ",                                 # drums medium
    6: " ··:·::∴",                       # drums small
}


# MIDIbox logo of the screensaver (seq_lcd_logo.inc): 2 character lines x 200 columns,
# bit n = pixel row n within the character line
LOGO = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 224, 240, 120, 252, 248, 128, 192, 224, 112, 184, 252, 254, 62, 14, 0, 0, 24, 24, 28, 140, 252, 124, 28, 12, 12, 4, 7, 3, 3, 134, 230, 246, 60, 28, 56, 240, 224, 0, 0, 0, 24, 24, 28, 12, 236, 252, 60, 12, 12, 12, 4, 4, 0, 128, 224, 240, 188, 142, 135, 3, 0, 0, 0, 0, 128, 128, 128, 128, 128, 128, 0, 0, 96, 224, 192, 0, 0, 0, 0, 128, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 96, 112, 248, 216, 200, 204, 204, 132, 134, 6, 6, 3, 0, 192, 192, 224, 248, 220, 204, 134, 6, 6, 6, 132, 196, 224, 96, 48, 48, 24, 24, 28, 204, 252, 16, 0, 0, 0, 0, 0, 0, 0, 28, 252, 0, 0, 0, 128, 224, 112, 60, 14, 7, 3, 1, 0, 128, 192, 0, 0, 224, 224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 48, 60, 31, 15, 3, 0, 3, 7, 3, 1, 0, 124, 63, 15, 1, 0, 96, 224, 96, 112, 120, 126, 111, 35, 48, 48, 112, 112, 64, 64, 104, 110, 111, 51, 48, 56, 24, 14, 7, 3, 64, 224, 96, 96, 112, 120, 127, 103, 48, 48, 48, 48, 48, 120, 126, 103, 99, 97, 49, 57, 29, 15, 2, 60, 62, 55, 35, 35, 33, 49, 49, 57, 31, 47, 48, 48, 24, 27, 15, 30, 126, 99, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 8, 60, 48, 96, 32, 32, 48, 48, 48, 25, 31, 15, 7, 8, 62, 63, 51, 33, 33, 32, 32, 32, 32, 8, 62, 63, 49, 48, 48, 55, 31, 30, 254, 255, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 63, 60, 15, 3, 1, 0, 0, 0, 0, 24, 30, 31, 27, 25, 248, 252, 63, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]


def screensaver_active(lcd):
    """Screensaver = the screen only contains spaces and custom chars (logo window)."""
    flat = [c for line in lcd for c in line]
    return any(c < 8 for c in flat) and all(c < 8 or c == 32 for c in flat)


class EmuLink:
    """Direct link to the MIDIbox SEQ emulator (emu_macos, TCP 127.0.0.1:8767) instead of MIDI ports:
    frames <port 0..3> <len hi> <len lo> <MIDI bytes>. Gives mido-like port objects (send / iter_pending)."""
    def __init__(self, host="127.0.0.1", port=8767):
        import socket, mido
        self.sock = socket.create_connection((host, port))
        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.parsers = [mido.Parser() for _ in range(4)]
        self.lock = threading.Lock()
        threading.Thread(target=self._rx, daemon=True).start()

    def _rx(self):
        buf = b""
        while True:
            d = self.sock.recv(65536)
            if not d:
                return
            buf += d
            while len(buf) >= 3:
                n = (buf[1] << 8) | buf[2]
                if len(buf) < 3 + n:
                    break
                port, data, buf = buf[0], buf[3:3 + n], buf[3 + n:]
                if port < 4:
                    self.parsers[port].feed(data)

    def send(self, port, msg):
        data = bytes(msg.bytes())
        with self.lock:
            self.sock.sendall(bytes([port, len(data) >> 8, len(data) & 0xFF]) + data)

    class Port:
        def __init__(self, link, n):
            self.link, self.n = link, n
        def send(self, msg):
            self.link.send(self.n, msg)
        def iter_pending(self):
            p = self.link.parsers[self.n]
            while p.pending():
                yield p.get_message()
        def close(self):
            pass

    def port(self, n):
        return EmuLink.Port(self, n)


class Seq:
    def __init__(self, in1, out1, out2, device_id=0):
        self.in1, self.out1, self.out2 = in1, out1, out2
        self.dev = device_id
        self.lock = threading.Lock()
        self.midi_lock = threading.Lock()
        self.btn_lock = threading.RLock()      # one complete button transition (state + its MIDI sequence) at a time
        self.fb_lock = threading.RLock()       # one file browser request / upload at a time (single reply list, single write handle on the SEQ)
        self.lcd = [[32] * 80 for _ in range(2)]
        self.charset = 0
        self.sr = [0] * 32
        self.matrix = [0] * 8
        self._cand = [None] * 8
        self.version = 0
        self.last_rx = 0.0
        self.held = []                        # buttons held down (remote keyboard)
        self.stop = threading.Event()

    # ------------------------------------------------------------- MIDI out
    def _send(self, port, msg):
        with self.midi_lock:
            port.send(msg)

    def remote_off(self):
        """Leave remote mode; otherwise the SEQ keeps catching all MIDI on Port 1."""
        import mido
        self._send(self.out1, mido.Message("sysex", data=SEQ_HDR + [self.dev, 0x09, 0x00, 0x00]))

    def refresh(self):
        import mido
        self._send(self.out1, mido.Message("sysex", data=SEQ_HDR + [self.dev, 0x09, 0x01]))

    def wake(self):
        """Leave the screensaver: remote buttons don't (only physical buttons do),
        the terminal command 'lcd' does (shows a short message for 2 s)."""
        with self.lock:
            active = screensaver_active(self.lcd)
        if active:
            self.terminal("lcd virtual panel")
            time.sleep(0.15)
        return active

    def button(self, name, down):
        with self.btn_lock:
            self._button(name, down)

    def _button(self, name, down):
        import mido
        name = name.upper()
        if down:
            self.wake()
        if name == "SAVE_ALL":
            if down:
                self.terminal("store")
            return
        note = BUTTON_NOTES.get(name)
        if note is None:
            raise KeyError(name)
        with self.lock:
            if down:
                if name in self.held:
                    return
                first = not self.held
                self.held.append(name)
            else:
                if name not in self.held:
                    return
                self.held.remove(name)
                last = not self.held
        if down:
            if first:
                self._send(self.out2, mido.Message("note_on", note=REMOTE_KEY, velocity=127))
                time.sleep(0.02)
            self._send(self.out2, mido.Message("note_on", note=note, velocity=127))
        else:
            self._send(self.out2, mido.Message("note_off", note=note))
            if last:
                time.sleep(0.02)
                self._send(self.out2, mido.Message("note_off", note=REMOTE_KEY))

    def release_all(self):
        with self.btn_lock:
            for b in list(self.held):
                self.button(b, False)

    def press(self, names, ms=100):
        names = [n.upper() for n in names]
        with self.btn_lock:                   # a combination is pressed and released as a whole
            for n in names:
                self.button(n, True)
                time.sleep(0.04)
            time.sleep(ms / 1000)
            for n in reversed(names):
                self.button(n, False)
                time.sleep(0.04)

    def encoder(self, name, delta):
        import mido
        idx = ENCODERS[name.upper()]
        self.wake()
        delta = max(-63, min(63, int(delta)))
        while delta:                                  # the firmware takes at most +-3 per CC (SEQ_UI_Encoder_Handler): split
            step = max(-3, min(3, delta))
            self._send(self.out1, mido.Message("control_change", channel=0, control=15 + idx, value=64 + step))
            delta -= step

    def terminal(self, cmd):
        import mido
        data = MIOS_HDR + [self.dev, 0x0D, 0x00] + [ord(c) & 0x7F for c in cmd] + [0x0A]
        self._send(self.out1, mido.Message("sysex", data=data))

    def fb(self, cmd, quiet=1.5, maxwait=20):
        """MIOS file browser (same protocol as MIOS Studio): send 'dir /SESSIONS', 'read ...' etc. and collect the replies."""
        import mido
        with self.fb_lock:
            self.fb_rx = []
            data = MIOS_HDR + [self.dev, 0x0D, 0x01] + [ord(c) & 0x7F for c in cmd] + [0x0A]
            self._send(self.out1, mido.Message("sysex", data=data))
            t0 = last = time.time()
            while time.time() - t0 < maxwait:
                time.sleep(0.05)
                if self.fb_rx and self.fb_last != last:
                    last = self.fb_last
                if self.fb_rx and time.time() - self.fb_last > quiet:
                    break
            return list(self.fb_rx)

    def _fb_cmd(self, cmd, timeout=5.0):
        """Send 1 browser command and wait for exactly 1 reply (every command gives one, except read/dir)."""
        import mido
        n = len(getattr(self, "fb_rx", []))
        self._send(self.out1, mido.Message("sysex", data=MIOS_HDR + [self.dev, 0x0D, 0x01] + [ord(c) & 0x7F for c in cmd] + [0x0A]))
        t0 = time.time()
        while time.time() - t0 < timeout:
            if len(self.fb_rx) > n:
                return self.fb_rx[n].rstrip("\x00")
            time.sleep(0.0005)
        return None

    def upload(self, src, name, block=32, burst=32):
        """Write all files of folder 'src' to /SESSIONS/<name> on the SEQ's SD card (the folder must not exist yet)."""
        import re
        import mido
        # validate everything before anything is created on the SD card
        block, burst = int(block), int(burst)
        if not 1 <= block <= 40:                      # "writedata XXXXXXXX " + hex data must stay below the 99 characters of the terminal
            raise ValueError("block must be 1..40")
        if not 1 <= burst <= 64:
            raise ValueError("burst must be 1..64")
        if not re.fullmatch(r"[A-Za-z0-9_-]{1,8}", name):
            raise ValueError("session name: 1..8 characters A-Z a-z 0-9 _ -")
        if not os.path.isdir(src):
            raise ValueError(f"{src} is not a folder")
        files = sorted(f for f in os.listdir(src) if os.path.isfile(os.path.join(src, f)) and not f.startswith("."))
        for f in files:
            if not re.fullmatch(r"[A-Za-z0-9_]{1,8}\.[A-Za-z0-9]{1,3}", f):
                raise ValueError(f"file name {f} is not a plain 8.3 name")
        with self.fb_lock:                            # the whole upload is one transaction
            listing = "".join(self.fb("dir /SESSIONS", 0.8)).replace("\x00", "") + ","
            if f",D{name}," in listing or f",F{name}," in listing:
                raise ValueError(f"/SESSIONS/{name} already exists")
            if self._fb_cmd(f"mkdir /SESSIONS/{name}") != "M#":
                raise ValueError("mkdir failed")
            t0, total = time.time(), 0
            for f in files:
                with open(os.path.join(src, f), "rb") as fh:
                    data = fh.read()
                if self._fb_cmd(f"write /SESSIONS/{name}/{f} {len(data)}") != "W00000000":
                    raise ValueError(f"write {f} failed")
                if not data:                          # empty file: the firmware closes it on a (empty) writedata
                    if self._fb_cmd("writedata 00000000") != "W#":
                        raise ValueError(f"closing empty file {f} failed")
                    continue
                pos = 0
                while pos < len(data):
                    cmds, wants = [], []
                    for _ in range(burst):            # burst: send blocks back to back, then check all replies
                        if pos >= len(data):
                            break
                        chunk = data[pos:pos + block]
                        cmds.append(f"writedata {pos:08X} " + chunk.hex().upper())
                        pos += len(chunk)
                        wants.append("W#" if pos >= len(data) else f"W{pos:08X}")
                    n = len(self.fb_rx)
                    for c in cmds:
                        self._send(self.out1, mido.Message("sysex", data=MIOS_HDR + [self.dev, 0x0D, 0x01] + [ord(ch) & 0x7F for ch in c] + [0x0A]))
                    tw = time.time()
                    while len(self.fb_rx) < n + len(cmds) and time.time() - tw < 10:
                        time.sleep(0.0005)
                    got = [r.rstrip("\x00") for r in self.fb_rx[n:n + len(cmds)]]
                    if got != wants:
                        bad = next(i for i, (g, w) in enumerate(zip(got + [None] * len(wants), wants)) if g != w)
                        raise ValueError(f"{f}: unexpected reply {got[bad] if bad < len(got) else None!r} at block {bad} of this burst, expected {wants[bad]!r} (offset {pos})")
                total += len(data)
            return {"bytes": total, "seconds": round(time.time() - t0, 1)}

    def sync(self):
        """Tap MENU -> the SEQ draws the whole screen again."""
        self.press(["MENU"], ms=250)

    # ------------------------------------------------------------- MIDI in
    def handle(self, msg):
        if msg.type != "sysex":
            return
        d = list(msg.data)
        if len(d) >= 7 and d[:4] == MIOS_HDR and d[5] == 0x0D and d[6] == 0x41:     # file browser reply
            if not hasattr(self, "fb_rx"):
                self.fb_rx = []
            self.fb_rx.append("".join(chr(b) for b in d[7:] if b < 0x80 and b != 0x0A))
            self.fb_last = time.time()
            return
        if len(d) < 7 or d[:4] != SEQ_HDR or d[5] != 0x09:
            return
        cmd = d[6]
        with self.lock:
            self.last_rx = time.time()
            if cmd == 0x02 and len(d) >= 9:                     # LCD
                x, y = d[7], d[8]
                if 0 <= y < 2:
                    for i, c in enumerate(d[9:]):
                        if x + i < 80:
                            self.lcd[y][x + i] = c
                    self.version += 1
            elif cmd == 0x03 and len(d) >= 8:                   # charset
                self.charset = d[7]
                self.version += 1
            elif cmd == 0x04 and len(d) >= 8:                   # LED shift registers
                first = d[7]
                for k, v in enumerate(d[8:]):
                    n = first + k
                    if n // 2 >= 32:
                        break
                    j = n // 2
                    self.sr[j] = (self.sr[j] & 0xF0) | v if n % 2 == 0 else (self.sr[j] & 0x0F) | (v << 4)
                sel = ~self.sr[0] & 0xFF                        # SR1: row select, active low
                if sel and not sel & (sel - 1):
                    row = sel.bit_length() - 1
                    val = self.sr[1]                            # SR2: LEDs of that row
                    # take it only when it is the same twice in a row (filters scan glitches)
                    if self._cand[row] == val and self.matrix[row] != val:
                        self.matrix[row] = val
                        self.version += 1
                    self._cand[row] = val

    def reader(self):
        while not self.stop.is_set():
            got = False
            for msg in self.in1.iter_pending():
                self.handle(msg)
                got = True
            if not got:
                time.sleep(0.002)

    def keepalive(self):
        while not self.stop.is_set():
            try:
                self.refresh()
            except Exception as e:                              # noqa: BLE001
                print("refresh failed:", e, file=sys.stderr)
            self.stop.wait(1.0)

    # ------------------------------------------------------------- state
    def leds(self):
        out = {}
        for name, (row, bit) in LEDS.items():
            out[name] = [bool(self.matrix[row] & (1 << bit)), False]
        for g in range(16):
            v = self.matrix[GP_ROW[g]]
            gm, rm = (0x80, 0x40) if g % 2 == 0 else (0x20, 0x10)
            out[f"GP{g + 1}"] = [bool(v & gm), bool(v & rm)]
        return out

    def screen_text(self):
        if screensaver_active(self.lcd):
            return ["[screensaver active - the next button/encoder wakes it up]".ljust(80), " " * 80]
        cs = CHARSET_TEXT.get(self.charset, "▒" * 8)
        def ch(c):
            if c < 8:
                return cs[c]
            if c == 0x7E:
                return "→"
            if c == 0x7F:
                return "←"
            return chr(c) if 32 <= c < 127 else " "
        with self.lock:
            return ["".join(ch(c) for c in line) for line in self.lcd]

    def state(self):
        with self.lock:
            return {
                "version": self.version,
                "lcd": [list(l) for l in self.lcd],
                "charset": self.charset,
                "leds": self.leds(),
                "online": time.time() - self.last_rx < 3,
                "held": list(self.held),
            }


# ---------------------------------------------------------------- HTTP
# routes that change something (buttons, encoders, files): POST only
MUTATING = {"/api/press", "/api/down", "/api/up", "/api/release_all", "/api/enc", "/api/sync", "/api/fb", "/api/upload", "/api/store"}


def make_handler(seq):
    class H(BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def _send(self, code, body, ctype="application/json"):
            if isinstance(body, (dict, list)):
                body = json.dumps(body)
            b = body.encode("utf-8")
            self.send_response(code)
            self.send_header("Content-Type", ctype + "; charset=utf-8")
            self.send_header("Content-Length", str(len(b)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(b)

        def _allowed(self):
            """Only requests from this panel itself: a web page on another site must not be able to press buttons,
            write to the SD card or delete files (cross-site forms/links, DNS rebinding)."""
            port = self.server.server_address[1]
            own = {f"127.0.0.1:{port}", f"localhost:{port}"}
            if self.headers.get("Host", "") not in own:
                return False
            origin = self.headers.get("Origin")
            if origin is not None and origin not in {"http://" + h for h in own}:
                return False
            if self.headers.get("Sec-Fetch-Site", "same-origin") not in ("same-origin", "none"):
                return False
            return True

        def do_POST(self):
            self._route(True)

        def do_GET(self):
            self._route(False)

        def _route(self, is_post):
            u = urlparse(self.path)
            q = {k: v[0] for k, v in parse_qs(u.query).items()}
            p = u.path
            if not self._allowed():
                return self._send(403, {"error": "forbidden (request not from this panel)"})
            if p in MUTATING and not is_post:
                return self._send(405, {"error": f"{p} changes something: use POST"})
            try:
                if p == "/":
                    return self._send(200, PAGE.replace("__LOGO__", json.dumps(LOGO)), "text/html")
                if p == "/events":
                    return self._events()
                if p == "/api/screen":
                    a, b = seq.screen_text()
                    sep = "+" + "-" * 40 + "+" + "-" * 40 + "+"
                    return self._send(200, f"{sep}\n|{a[:40]}|{a[40:]}|\n|{b[:40]}|{b[40:]}|\n{sep}\n",
                                      "text/plain")
                if p == "/api/state":
                    s = seq.state()
                    s["screen"] = seq.screen_text()
                    return self._send(200, s)
                if p == "/api/buttons":
                    return self._send(200, {"buttons": sorted(BUTTON_NOTES) + ["SAVE_ALL"],
                                            "encoders": list(ENCODERS),
                                            "unavailable": sorted(UNAVAILABLE)})
                if p == "/api/press":
                    names = [n for n in q.get("b", "").replace(" ", "+").split("+") if n]
                    self._check(names)
                    seq.press(names, int(q.get("ms", 100)))
                    time.sleep(float(q.get("wait", 0.15)))      # give the SEQ some time
                    return self._send(200, {"ok": True, "pressed": names})
                if p in ("/api/down", "/api/up"):
                    names = [q.get("b", "")]
                    self._check(names)
                    seq.button(names[0], p == "/api/down")
                    return self._send(200, {"ok": True})
                if p == "/api/release_all":
                    seq.release_all()
                    return self._send(200, {"ok": True})
                if p == "/api/enc":
                    e = q.get("e", "").upper()
                    if e not in ENCODERS:
                        return self._send(400, {"error": f"unknown encoder {e}", "encoders": list(ENCODERS)})
                    seq.encoder(e, int(q.get("d", 1)))
                    time.sleep(float(q.get("wait", 0.1)))
                    return self._send(200, {"ok": True})
                if p == "/api/sync":
                    seq.sync()
                    time.sleep(0.3)
                    return self._send(200, {"ok": True})
                if p == "/api/fb":
                    return self._send(200, {"reply": seq.fb(q["cmd"], float(q.get("quiet", 1.5)), float(q.get("maxwait", 20)))})
                if p == "/api/upload":
                    return self._send(200, seq.upload(q["src"], q["name"], int(q.get("block", 32)), int(q.get("burst", 32))))
                if p == "/api/store":
                    seq.terminal("store")
                    return self._send(200, {"ok": True})
                return self._send(404, {"error": "not found"})
            except KeyError as e:
                return self._send(400, {"error": f"unknown button {e}",
                                        "hint": "see /api/buttons"})
            except ValueError as e:
                return self._send(400, {"error": str(e)})

        def _check(self, names):
            for n in names:
                n = n.upper()
                if n in UNAVAILABLE:
                    raise ValueError(f"{n} can't be operated remotely (not in the firmware's remote keyboard map)")
                if n not in BUTTON_NOTES and n not in SPECIAL_BUTTONS:
                    raise KeyError(n)
            if not names:
                raise ValueError("no button given (?b=NAME)")

        def _events(self):
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            last = None
            try:
                while not seq.stop.is_set():
                    s = seq.state()
                    key = (s["version"], s["online"], tuple(s["held"]))
                    if key != last:
                        last = key
                        self.wfile.write(b"data: " + json.dumps(s).encode() + b"\n\n")
                        self.wfile.flush()
                    time.sleep(0.03)
            except (BrokenPipeError, ConnectionResetError):
                pass
    return H


def find_port(names, wanted):
    for n in names:
        if wanted.lower() in n.lower():
            return n
    return None


def main():
    ap = argparse.ArgumentParser(description="MIDIbox SEQ V4+ virtual front panel")
    ap.add_argument("--port1", default="MIDIbox SEQ V4+ Port 1", help="remote/LCD port (USB1)")
    ap.add_argument("--port2", default="MIDIbox SEQ V4+ Port 2", help="port for the buttons (remote keyboard)")
    ap.add_argument("--http", type=int, default=8765, help="HTTP port (default 8765)")
    ap.add_argument("--no-browser", action="store_true")
    ap.add_argument("--device-id", type=int, default=0)
    ap.add_argument("--emu", action="store_true", help="MIDIbox SEQ emulator (emu_macos, run.sh) via 127.0.0.1:8767")
    args = ap.parse_args()

    try:
        import mido
    except ImportError:
        sys.exit("mido is missing: pip3 install mido python-rtmidi")

    if args.emu:                                        # MIDIbox SEQ emulator (emu_macos, run.sh): direct link, no MIDI ports
        link = EmuLink()
        in1, out1, out2 = link.port(0), link.port(0), link.port(1)
    else:
        ins, outs = mido.get_input_names(), mido.get_output_names()
        p_in1, p_out1, p_out2 = find_port(ins, args.port1), find_port(outs, args.port1), find_port(outs, args.port2)
        if not (p_in1 and p_out1 and p_out2):
            sys.exit(f"MIDIbox ports not found. IN: {ins}  OUT: {outs}")
        in1, out1, out2 = mido.open_input(p_in1), mido.open_output(p_out1), mido.open_output(p_out2)
    seq = Seq(in1, out1, out2, args.device_id)
    threading.Thread(target=seq.reader, daemon=True).start()
    threading.Thread(target=seq.keepalive, daemon=True).start()

    def initial_sync():
        time.sleep(0.6)
        seq.sync()
    threading.Thread(target=initial_sync, daemon=True).start()

    import signal
    signal.signal(signal.SIGTERM, lambda *a: (_ for _ in ()).throw(KeyboardInterrupt()))

    srv = ThreadingHTTPServer(("127.0.0.1", args.http), make_handler(seq))
    srv.daemon_threads = True
    url = f"http://127.0.0.1:{args.http}/"
    print(f"MBSEQ panel running on {url}  (Ctrl+C = stop)", flush=True)
    if not args.no_browser:
        threading.Timer(0.8, lambda: webbrowser.open(url)).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        seq.stop.set()
        seq.release_all()
        time.sleep(0.05)
        seq.remote_off()
        time.sleep(0.05)
        srv.server_close()
        import os
        os._exit(0)                          # don't let rtmidi threads hang


# ---------------------------------------------------------------- HTML
PAGE = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>MIDIbox SEQ V4+ – virtual front panel</title>
<style>
:root{--txt:#d5d9de;--dim:#8a9199;--lcd:#9fd400;--lcdbg:#1a2a06;--col:66px}
*{box-sizing:border-box}
html,body{margin:0;background:#111;height:100%}
body{color:var(--txt);font:13px/1.2 -apple-system,Helvetica,Arial,sans-serif;user-select:none;
     display:flex;justify-content:center;align-items:flex-start;padding:18px;overflow:auto}
#wrap{transform-origin:top center}
.panel{background:linear-gradient(#26292d,#1b1d20);border-radius:14px;padding:16px 22px 18px;
       box-shadow:0 10px 40px #000a, inset 0 1px 0 #ffffff14;display:inline-block}
.top{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}
.brand{font-weight:700;letter-spacing:.12em;color:#e8eaed}
.status{font-size:12px;color:var(--dim)} .status b.on{color:#6c6} .status b.off{color:#e55}
.body{display:flex;gap:26px;align-items:stretch}
/* ---------- left section ---------- */
.left{display:flex;flex-direction:column;justify-content:space-between;padding:2px 0 14px}
.pair{display:flex;gap:16px}
.left .ctl{gap:2px}
.left .ctl .lbl{display:none}
.left .ctl .bline .side{display:none}
.colblk{display:flex;gap:6px;align-items:stretch}
.vlabel{writing-mode:vertical-rl;transform:rotate(180deg);font-size:10px;letter-spacing:.14em;color:var(--dim);
        border-left:1px solid #ffffff30;padding-left:5px;text-align:center;white-space:nowrap}
.stack{display:flex;flex-direction:column;gap:5px}
/* ---------- main grid ---------- */
.main{display:grid;grid-template-columns:repeat(8,var(--col)) 84px repeat(8,var(--col));
      grid-template-rows:auto 64px auto 62px 62px;align-items:center;justify-items:center}
.lcd{background:#0b0f05;border-radius:6px;padding:9px;box-shadow:inset 0 0 0 3px #000,0 0 0 1px #444;
     justify-self:stretch;margin:0 4px 6px}
.lcd .scr{background:var(--lcdbg);padding:7px 6px;border-radius:2px;
          font:19px/25px "SF Mono",Menlo,Consolas,monospace;color:var(--lcd);white-space:pre;text-shadow:0 0 6px #9fd40055}
.lcd .row{height:25px;display:flex;justify-content:center}
.lcd .c{display:inline-block;width:12.3px;height:25px;text-align:center}
.lcd .c.g{background-size:100% 100%;background-repeat:no-repeat}
/* ---------- controls ---------- */
.enc{width:44px;height:44px;border-radius:50%;cursor:ns-resize;position:relative;
     background:radial-gradient(circle at 35% 30%,#5a5f66,#2a2d31 60%,#16181a);
     box-shadow:0 3px 6px #000c, inset 0 1px 1px #ffffff22}
.enc::after{content:"";position:absolute;left:50%;top:5px;width:3px;height:11px;margin-left:-1.5px;
            background:#ddd;border-radius:2px;transform-origin:50% 17px;transform:rotate(var(--a,0deg))}
.enc.big{width:84px;height:84px}
.enc.big::after{top:8px;height:20px;transform-origin:50% 34px}
.ctl{display:flex;flex-direction:column;align-items:center;gap:3px;position:relative}
.ctl .topl{display:flex;align-items:center;gap:5px;height:13px;font-size:11px;color:var(--txt)}
.ctl .bline{display:flex;align-items:center;gap:4px}
.ctl .side{font-size:11px;width:9px;text-align:right;color:var(--txt)}
.ctl .side.empty{visibility:hidden}
.ctl .lbl{font-size:10px;color:var(--txt);letter-spacing:.04em;white-space:nowrap;min-height:12px;text-align:center}
.led{width:9px;height:9px;border-radius:50%;background:#2b1a1a;box-shadow:inset 0 1px 2px #000}
.led.nl{visibility:hidden}
.led.g{background:#3cff5a;box-shadow:0 0 8px #3cff5a,0 0 2px #fff inset}
.led.r{background:#ff3b30;box-shadow:0 0 8px #ff3b30,0 0 2px #fff inset}
.led.y{background:#ffc400;box-shadow:0 0 8px #ffc400,0 0 2px #fff inset}
.led.on{background:#ff5a2a;box-shadow:0 0 8px #ff5a2a,0 0 2px #fff inset}
button.b{width:42px;height:24px;border-radius:4px;border:0;cursor:pointer;color:#ccc;font-size:10px;
         background:linear-gradient(#4a4e54,#33363b);box-shadow:0 3px 0 #111,0 4px 6px #0008;padding:0}
button.b:active,button.b.down{transform:translateY(2px);box-shadow:0 1px 0 #111}
button.b.latched{outline:2px solid #ffc400}
button.b.na{opacity:.35;cursor:not-allowed}
.beat{grid-column:9;grid-row:3}
.dw{grid-column:9;grid-row:4 / span 2}
.hint{margin-top:12px;font-size:11px;color:var(--dim);max-width:1300px}
</style></head><body>
<div id="wrap"><div class="panel">
  <div class="top"><div class="brand">MIDIbox SEQ V4+ · virtual front panel</div>
       <div class="status">SEQ: <b id="st" class="off">offline</b> · charset <span id="cs">?</span>
       · <a href="#" id="sync" style="color:#9cf">refresh screen</a></div></div>
  <div class="body">
    <div class="left" id="left"></div>
    <div class="main" id="main"></div>
  </div>
  <div class="hint">Click = press a button (can be held) · right click = latch a button for combinations
   (e.g. MENU or SELECT + GP) · encoders: scroll or drag vertically, Shift = faster ·
   keys: ←/→ datawheel, Enter = SELECT, Esc = EXIT · grey buttons can't be operated remotely by the firmware.</div>
</div></div>
<script>
const CHARSETS={ // 8 chars x 8 rows, 5 bits wide (seq_lcd.c)
 1:[[1,3,7,3,1,0,0,0],[0,0,0,16,24,28,24,16],[1,3,7,19,25,28,24,16]],
 2:[0,1,2,3,4,5,6,7].map(n=>Array.from({length:8},(_,r)=>r>=7-n?30:0)),
 3:[[0,0,0,0,0,0,0,0],[0,16,16,16,16,16,16,0],[0,20,20,20,20,20,20,0],[0,21,21,21,21,21,21,0],
    [0,0,0,16,16,0,0,0],[0,16,20,21,21,20,16,0]],
 4:[[0,0,0,4,4,0,0,0],[0,4,14,31,31,14,4,0],[0,14,17,17,17,17,14,0],[0,14,17,21,21,17,14,0]],
 5:[[0,0,0,0,0,0,0,0],[0,0,0,8,8,0,0,0],[0,0,0,2,2,0,0,0],[0,0,0,10,10,0,0,0]],
 6:[[0,0,0,0,0,0,0,0],[0,0,0,16,16,0,0,0],[0,0,0,4,4,0,0,0],[0,0,0,20,20,0,0,0],
    [0,0,0,1,1,0,0,0],[0,0,0,17,17,0,0,0],[0,0,0,20,20,0,0,0],[0,0,0,21,21,0,0,0]]};
const glyphCache={};
function glyph(cs,code){
  const k=cs+':'+code; if(glyphCache[k]) return glyphCache[k];
  const bm=(CHARSETS[cs]||[])[code]; let r='';
  if(bm){ bm.forEach((v,y)=>{for(let x=0;x<5;x++) if(v&(16>>x)) r+=`<rect x="${x}" y="${y}" width=".9" height=".9"/>`;}); }
  else r='<rect x="0" y="0" width="5" height="8" opacity=".25"/>';
  const svg=`<svg xmlns="http://www.w3.org/2000/svg" viewBox="-.5 -.5 6 9" fill="#9fd400">${r}</svg>`;
  return glyphCache[k]='url("data:image/svg+xml;utf8,'+encodeURIComponent(svg)+'")';
}
const LOGO=__LOGO__;
function logoGlyph(x,y){ // character at column x (0..39 within one LCD), line y
  const k='logo:'+x+':'+y; if(glyphCache[k]) return glyphCache[k]; let r='';
  for(let cx=0;cx<5;cx++){const v=LOGO[y*200+x*5+cx]||0;
    for(let py=0;py<8;py++) if(v&(1<<py)) r+=`<rect x="${cx}" y="${py}" width=".9" height=".9"/>`;}
  const svg=`<svg xmlns="http://www.w3.org/2000/svg" viewBox="-.5 -.5 6 9" fill="#9fd400">${r}</svg>`;
  return glyphCache[k]='url("data:image/svg+xml;utf8,'+encodeURIComponent(svg)+'")';
}
const api=(p)=>fetch(p,{method:'POST'}).catch(()=>{});
const NA=new Set(['SCRUB','LIVE','RECORD','COPY','PASTE','CLEAR']);
const latched=new Set(), ledEls={};
const el=(tag,cls,txt)=>{const e=document.createElement(tag);if(cls)e.className=cls;if(txt!=null)e.textContent=txt;return e;};

/* button: opts = {top:'1', led:true|false, side:'A', label:'MIXER', ledLeft:true} */
function ctl(name,o={}){
  const w=el('div','ctl'); const led=el('div','led'+(o.led===false?' nl':''));
  if(o.led!==false) ledEls[name]=led;
  const topl=el('div','topl'); if(o.top!=null) topl.append(o.ledLeft?led:'',el('span','',o.top),o.ledLeft?'':led); else topl.append(led);
  const b=el('button','b'+(NA.has(name)?' na':''));
  b.title=NA.has(name)?name+': can\'t be operated remotely':name;
  const bl=el('div','bline'); bl.append(el('span','side'+(o.side?'':' empty'),o.side||'·'),b,el('span','side empty','·'));
  w.append(topl,bl,el('div','lbl',o.label||''));
  if(!NA.has(name)){
    let isDown=false;
    b.addEventListener('pointerdown',e=>{if(e.button!==0)return;e.preventDefault();b.setPointerCapture(e.pointerId);
      if(latched.has(name)){latched.delete(name);b.classList.remove('latched');api('/api/up?b='+name);return;}
      isDown=true;b.classList.add('down');api('/api/down?b='+name);});
    const up=()=>{if(!isDown)return;isDown=false;b.classList.remove('down');api('/api/up?b='+name);};
    b.addEventListener('pointerup',up);b.addEventListener('pointercancel',up);
    b.addEventListener('contextmenu',e=>{e.preventDefault();
      if(latched.has(name)){latched.delete(name);b.classList.remove('latched');api('/api/up?b='+name);}
      else{latched.add(name);b.classList.add('latched');api('/api/down?b='+name);}});
  }
  return w;
}
function encoder(name,big){
  const e=el('div','enc'+(big?' big':''));e.title=name;
  let ang=0,acc=0,dragY=null,pending=0,timer=null;
  const send=d=>{ang+=d*15;e.style.setProperty('--a',ang+'deg');pending+=d;
    if(!timer)timer=setTimeout(()=>{const p=pending;pending=0;timer=null;if(p)api(`/api/enc?e=${name}&d=${p}&wait=0`);},30);};
  e.addEventListener('wheel',ev=>{ev.preventDefault();acc+=ev.deltaY;const step=ev.shiftKey?15:40;
    while(Math.abs(acc)>=step){const d=acc<0?1:-1;send(ev.shiftKey?d*4:d);acc-=(acc<0?-step:step);}},{passive:false});
  e.addEventListener('pointerdown',ev=>{dragY=ev.clientY;e.setPointerCapture(ev.pointerId);});
  e.addEventListener('pointermove',ev=>{if(dragY===null)return;const dy=dragY-ev.clientY;const step=ev.shiftKey?3:8;
    if(Math.abs(dy)>=step){const n=Math.trunc(dy/step);send(ev.shiftKey?n*4:n);dragY-=n*step;}});
  e.addEventListener('pointerup',()=>dragY=null);
  return e;
}

/* ---------- left section (as on Wilba's panel) ---------- */
function column(title,items){const c=el('div','colblk');c.append(el('div','vlabel',title));
  const s=el('div','stack');items.forEach(i=>s.appendChild(i));c.appendChild(s);return c;}
const left=document.getElementById('left');
const p1=el('div','pair');
p1.append(column('TRACK GROUP',[1,2,3,4].map(i=>ctl('GROUP'+i,{top:String(i),ledLeft:true}))),
          column('TRACK',[1,2,3,4].map(i=>ctl('TRACK'+i,{top:String(i),ledLeft:true}))));
const p2=el('div','pair');
p2.append(column('TRIGGER LAYER',['A','B','C'].map(x=>ctl('TRG_'+x,{top:x,ledLeft:true}))),
          column('PARAMETER LAYER',['A','B','C'].map(x=>ctl('PAR_'+x,{top:x,ledLeft:true}))));
left.append(p1,p2);

/* ---------- main grid ---------- */
const main=document.getElementById('main');
const put=(node,col,row,span)=>{node.style.gridColumn=span?`${col} / span ${span}`:col;node.style.gridRow=row;main.appendChild(node);return node;};
const colOf=i=>i<8?i+1:i+2;   // GP index 0..15 -> grid column (column 9 = middle)
// LCDs
const cells=[[],[]];
[0,1].forEach(d=>{const l=el('div','lcd'),scr=el('div','scr');l.appendChild(scr);
  for(let y=0;y<2;y++){const row=el('div','row');scr.appendChild(row);
    for(let x=0;x<40;x++){const s=el('span','c',' ');row.appendChild(s);cells[y][d*40+x]=s;}}
  put(l,d?10:1,1,8);});
// encoders + GP buttons
const GP_SIDE=['A','B','C','D','E','F','G','H','1','2','3','4','5','6','7','8'];
const GP_LBL=['MIXER','EVENT','MODE','DIRECTION','DIVIDER','LENGTH','TRANSPOSE','GROOVE',
              'TRIGGER','FX','MANUAL','MORPH','BPM','SAVE','MIDI','SYS EX'];
for(let i=0;i<16;i++){
  put(encoder('GP'+(i+1)),colOf(i),2);
  put(ctl('GP'+(i+1),{top:String(i+1),side:GP_SIDE[i],label:GP_LBL[i]}),colOf(i),3);
}
// middle: BEAT + datawheel
const beat=el('div','ctl beat');const bl=el('div','led');ledEls['BEAT']=bl;
beat.append(el('div','topl'),bl,el('div','lbl','BEAT'));main.appendChild(beat);
const dw=encoder('DATAWHEEL',true);dw.classList.add('dw');main.appendChild(dw);
// bottom rows left (column 1..7)
[['EDIT','EDIT',1],['MUTE','MUTE',1],['PATTERN','PATTERN',1],['SONG','SONG',1],
 ['COPY','COPY',0],['PASTE','PASTE',0],['CLEAR','CLEAR',0]]
 .forEach(([n,l,led],i)=>put(ctl(n,{label:l,led:!!led}),i+1,4));
[['STEP_VIEW','STEP VIEW',1],['SOLO','SOLO',1],['FAST','FAST',1],['ALL','ALL',1],
 ['MENU','MENU',0],['SELECT','SELECT',0],['EXIT','EXIT',0]]
 .forEach(([n,l,led],i)=>put(ctl(n,{label:l,led:!!led}),i+1,5));
// bottom rows right (columns of GP10..16)
[['DOWN','▽',0],['UP','△',0],['UTILITY','UTILITY',0],['TRACK_SEL','F1',0],['LIVE','F2',0],
 ['RECORD','F3',0],['SAVE_ALL','F4',0]]
 .forEach(([n,l,led],i)=>put(ctl(n,{label:l,led:!!led}),colOf(9+i),4));
[['SCRUB','SCRUB',0],['METRONOME','METRONOME',0],['REW','◁◁',1],['STOP','□',1],['PAUSE','❚❚',1],
 ['PLAY','▷',1],['FWD','▷▷',1]]
 .forEach(([n,l,led],i)=>put(ctl(n,{label:l,led:!!led}),colOf(9+i),5));

/* ---------- draw the LCDs ---------- */
const prev=[[],[]];let prevCs=-1;
let prevSaver=false;
function drawLcd(lcd,cs){
  const flat=lcd[0].concat(lcd[1]);
  const saver=flat.some(c=>c<8)&&flat.every(c=>c<8||c===32);
  const force=saver!==prevSaver; prevSaver=saver;
  for(let y=0;y<2;y++)for(let x=0;x<80;x++){const c=lcd[y][x];
    if(!force&&prev[y][x]===c&&!(c<8&&cs!==prevCs))continue; prev[y][x]=c; const e=cells[y][x];
    if(c<8){e.className='c g';e.textContent='';e.style.backgroundImage=saver?logoGlyph(x%40,y):glyph(cs,c);}
    else{e.className='c';e.style.backgroundImage='';
      e.textContent=c===0x7e?'→':c===0x7f?'←':(c>=32&&c<127?String.fromCharCode(c):' ');}}
  prevCs=cs;
}
/* ---------- scale to the window width ---------- */
function fit(){const w=document.getElementById('wrap');w.style.transform='';
  const s=Math.min(1,(innerWidth-36)/w.scrollWidth);w.style.transform=`scale(${s})`;}
addEventListener('resize',fit);setTimeout(fit,0);

/* ---------- keyboard ---------- */
addEventListener('keydown',e=>{if(e.repeat&&!['ArrowLeft','ArrowRight'].includes(e.key))return;
  if(e.key==='ArrowRight')api('/api/enc?e=DATAWHEEL&d=1&wait=0');
  else if(e.key==='ArrowLeft')api('/api/enc?e=DATAWHEEL&d=-1&wait=0');
  else if(e.key==='Enter')api('/api/press?b=SELECT&wait=0');
  else if(e.key==='Escape')api('/api/press?b=EXIT&wait=0');});
document.getElementById('sync').onclick=e=>{e.preventDefault();api('/api/sync');};

/* ---------- live updates ---------- */
const es=new EventSource('/events');
es.onmessage=ev=>{const s=JSON.parse(ev.data);drawLcd(s.lcd,s.charset);
  document.getElementById('cs').textContent=s.charset||'?';
  const st=document.getElementById('st');st.textContent=s.online?'online':'offline';st.className=s.online?'on':'off';
  for(const[n,[g,r]]of Object.entries(s.leds)){const e=ledEls[n];if(!e)continue;
    e.className='led '+(n.startsWith('GP')?(g&&r?'y':g?'g':r?'r':''):(g?'on':''));}};
es.onerror=()=>{const st=document.getElementById('st');st.textContent='server gone';st.className='off';};
</script></body></html>
"""

if __name__ == "__main__":
    main()
