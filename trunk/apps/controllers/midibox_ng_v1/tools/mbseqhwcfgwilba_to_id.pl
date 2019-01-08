#!/usr/bin/perl -w
# $Id$
# This script outputs the expected IDs for Wilba's Frontpanel
#
# Call this script with:
# perl < $MIOS32_PATH/tools/apps/sequencers/midibox_seq_v4/hwcfg/wilba/MBSEQ_HW.V4

while( <STDIN> ) {
  if( /^(BUTTON_\S+)\s+(\d+)\s+(\d+)/ ||
      /^(LED_\S+)\s+(\d+)\s+(\d+)/ ) {
    my $name = $1;
    my $sr = $2;
    my $pin = $3;

    if( $sr ) {
      printf "${name}: id=%d\n", 8*($sr-16-1) + $pin + 1;
    }
  }
}
