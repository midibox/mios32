#!/usr/bin/perl

# originally developed for MBFM frequency calculation (see there)
# using tempered scale
# it's 3'o'clock in the morning, currently I don't know an easier way to calculate the table like shown here...

my $fact = 1.059463; # 12rt2

my @note_name = ("C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-");

my $i;
my $a_freq = 13.75;
for($x=0; $x<128; $x+=12)
{
    my $octave  = int($x / 12);
    my @note = ();

    $note[9]  = $a_freq;
    for($i=10; $i<=11; ++$i) { $note[$i] = $note[$i-1] * $fact; }
    for($i=8; $i>=0; --$i)   { $note[$i] = $note[$i+1] / $fact; }

    for($i=0; $i<12; ++$i) {
      printf "  %10.5f,   // %s%d\n", $note[$i], $note_name[$i], $octave;
    }

    $a_freq *= 2;
}
