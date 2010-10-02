#!/usr/bin/perl
# $Id$

printf "// generated with 'perl hz_v_table.pl > aout_hz_v_table.inc'\n";
printf "static u16 hz_v_table[128] = {\n";

int i;
my $fact = 1.059463; # 12rt2
my $v    = 0.125;    # voltage at C-0
my @str = ("C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "H-");

for($i=0; $i<128; ++$i)
{
  my $octave  = int($i / 12);
  my @note = ();

  my $word = $v / 0.00015625;
  if( $word > 0xffff ) { $word = 0xffff; }
  printf "\t0x%04x, // %s%d: %6.3fV\n", $word, $str[$i % 12], $octave, $v;

  $v *= $fact;
}

printf "};\n";
