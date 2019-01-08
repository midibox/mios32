#!/usr/bin/perl -w
# $Id: kb_vel_map.pl 1689 2013-02-09 22:50:21Z tk $
# this script outputs a velocity map for cfg/tests/kb_2.ngc

my @map = ();

####################################################################################################
for($i=0; $i<128; ++$i) {
  my $value;
  my $min;
  my $max;
  my $normalized;
  my $range;

  if( $i < 32 ) {
    $min = 0;
    $max = 63;
    $normalized = $i;
    $range = 31 - 0 + 1;
  } elsif( $i < 64 ) {
    $min = 64;
    $max = 95;
    $normalized = $i - 32;
    $range = 63 - 32 + 1;
  } else {
    $min = 96;
    $max = 127;
    $normalized = $i - 64;
    $range = 127 - 64 + 1;
  }

  $value = $min + ($max-$min+1) * ($normalized / $range);

  push @map, $value;
}


####################################################################################################

for($i=0; $i<128; $i += 16) {
  my $values = "";
  for($j=0; $j<16; ++$j) {
    $values .= sprintf(" %3d", $map[$i + $j]);
  }

  printf("%-4s %s%s\n", ($i == 0) ? "MAP1" : "", $values, ($i < 128-16) ? " \\" : "");
}

####################################################################################################
open(OUT_DAT, ">vel.dat") || die;

for($i=0; $i<128; ++$i) {
  printf OUT_DAT "%d\t%d\n", $i, $map[$i];
}

close(OUT_DAT);

####################################################################################################
system("gnuplot --persist -e \"plot 'vel.dat' title 'Velocity' with lines\"") && die;


