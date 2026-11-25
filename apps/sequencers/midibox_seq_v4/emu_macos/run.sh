#!/bin/sh
# Starts the MIDIbox SEQ emulator + the virtual panel (http://127.0.0.1:8766/) and stops both with Ctrl-C.
# The panel talks to the emulator over 127.0.0.1:8767 (not via CoreMIDI). Arguments are passed to mbseq_emu.
# NO_BROWSER=1 ./run.sh  does not open the browser.
cd "$(dirname "$0")"
[ -x mbseq_emu ] || make || exit 1
NEW_SD=
[ -e sdcard.img ] || { ./make_sdcard.sh || exit 1; NEW_SD=1; }
./mbseq_emu "$@" &
EMU=$!
until nc -z 127.0.0.1 8767 2>/dev/null; do kill -0 $EMU 2>/dev/null || exit 1; sleep 0.3; done
sleep 3     # let the SEQ finish booting (session load) before the panel taps MENU to fetch the screen
if [ -n "$NEW_SD" ]; then   # fresh SD card: let the SEQ create a session "EMU" (terminal command "new EMU", as on the real SEQ)
  python3 -c "import mido,time,os; o=mido.open_output('MBSEQ EMU Port 1'); o.send(mido.Message('sysex', data=[0,0,0x7e,0x32,0,0x0d,0]+[ord(c) for c in 'new EMU']+[0x0a])); time.sleep(0.3); os._exit(0)"
  sleep 20
fi
python3 -u ../panel/mbseq_panel.py --emu --http 8766 ${NO_BROWSER:+--no-browser} &
PANEL=$!
trap 'kill $PANEL $EMU 2>/dev/null; wait; exit 0' INT TERM
wait $EMU
kill $PANEL 2>/dev/null
