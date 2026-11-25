#!/bin/sh
# Creates the emulator's SD card: sdcard.img = 256 MB raw FAT32 image with the folders the SEQ uses
# (/SESSIONS, /MIDI, /PRESETS, /SYSEX) and the hardware config of Wilba's front panel (../hwcfg/wilba/MBSEQ_HW.V4),
# which is the layout the virtual panel shows.
#   ./make_sdcard.sh                      fresh card
#   ./make_sdcard.sh /Volumes/MBSEQ       a copy of a real SD card (card in the Mac's reader)
# Look into the image in Finder:  hdiutil attach -imagekey diskimage-class=CRawDiskImage sdcard.img
# (eject it again before starting the emulator)
set -e
cd "$(dirname "$0")"
IMG=sdcard.img
SRC=$1
[ -e "$IMG" ] && { echo "$IMG exists - remove it first (rm $IMG)"; exit 1; }
mkfile -n 256m "$IMG"
DEV=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount "$IMG" | awk 'NR==1{print $1}')
newfs_msdos -F 32 -v MBSEQ "$DEV" > /dev/null
hdiutil detach "$DEV" > /dev/null
MNT=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage "$IMG" | awk -F'\t' 'END{print $NF}')
mkdir -p "$MNT/SESSIONS" "$MNT/MIDI" "$MNT/PRESETS" "$MNT/SYSEX"
cp ../hwcfg/wilba/MBSEQ_HW.V4 "$MNT/"
if [ -n "$SRC" ]; then cp -R "${SRC%/}"/ "$MNT"/ 2>/dev/null || true; echo "copied $SRC"; fi
find "$MNT" -maxdepth 2 -not -name ".*" | sed "s#$MNT#  #"
hdiutil detach "$MNT" > /dev/null
echo "created $IMG"
