#!/bin/sh
# Creates the emulator's SD card: sdcard.img = 256 MB raw FAT32 image with the folders the SEQ uses
# (/SESSIONS, /MIDI, /PRESETS, /SYSEX) and the hardware config of Wilba's front panel (../hwcfg/wilba/MBSEQ_HW.V4),
# which is the layout the virtual panel shows.
#   ./make_sdcard.sh                      fresh card
#   ./make_sdcard.sh /Volumes/MBSEQ       a copy of a real SD card (card in the Mac's reader)
# Look into the image in Finder:  hdiutil attach -imagekey diskimage-class=CRawDiskImage sdcard.img
# (eject it again before starting the emulator)
# If anything fails, the image is detached and removed again, so the next try starts clean.
cd "$(dirname "$0")" || exit 1
IMG=sdcard.img
SRC=$1
[ -e "$IMG" ] && { echo "$IMG exists - remove it first (rm $IMG)"; exit 1; }
[ -z "$SRC" ] || [ -d "$SRC" ] || { echo "source folder '$SRC' doesn't exist"; exit 1; }

DEV=; MNT=
cleanup() {
  [ -n "$MNT" ] && hdiutil detach "$MNT" > /dev/null 2>&1
  [ -n "$DEV" ] && hdiutil detach "$DEV" > /dev/null 2>&1
  rm -f "$IMG"
}
fail() { echo "make_sdcard.sh: $1" >&2; cleanup; exit 1; }

mkfile -n 256m "$IMG" || fail "can't create $IMG"
DEV=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount "$IMG" | awk 'NR==1{print $1}')
[ -n "$DEV" ] || fail "can't attach $IMG"
newfs_msdos -F 32 -v MBSEQ "$DEV" > /dev/null || fail "newfs_msdos failed"
hdiutil detach "$DEV" > /dev/null || fail "can't detach $DEV"
DEV=
MNT=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage "$IMG" | awk -F'\t' 'END{print $NF}')
[ -d "$MNT" ] || fail "can't mount $IMG"
mkdir -p "$MNT/SESSIONS" "$MNT/MIDI" "$MNT/PRESETS" "$MNT/SYSEX" || fail "can't create folders"
cp ../hwcfg/wilba/MBSEQ_HW.V4 "$MNT/" || fail "can't copy MBSEQ_HW.V4"
if [ -n "$SRC" ]; then
  cp -R "${SRC%/}"/ "$MNT"/ || fail "copy from $SRC failed (card full or unreadable files?)"
  echo "copied $SRC"
fi
find "$MNT" -maxdepth 2 -not -name ".*" | sed "s#$MNT#  #"
hdiutil detach "$MNT" > /dev/null || fail "can't detach $MNT"
echo "created $IMG"
