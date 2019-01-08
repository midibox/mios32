#!/usr/bin/perl
# $Id: patterns.pl 1600 2012-12-14 00:17:22Z tk $
# used to generate patterns in src/mbng_matrix.c

my $taken_line = 0;
while( <STDIN> ) {
  if( /^\s*\/\// ) {
    if( $taken_line ) {
      $taken_line = 0;
      print "\n";
    }
  } elsif( /b'(.*)'/ ) {
    my $pattern = $1;
    my $dec = bin2dec($pattern);
    ++$taken_line;

    printf "    0x%04x, // [%2d] b'${pattern}'%s\n", bin2dec($pattern), $taken_line-1, $taken_line == 9 ? " // taken when mid value has been selected" : "";
  }
}

sub bin2dec {
  return unpack("N", pack("B32", substr("0" x 32 . shift, -32)));
}
