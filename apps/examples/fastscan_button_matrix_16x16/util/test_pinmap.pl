#!/usr/bin/perl -w
# $Id$
# this short perl script checks if the calculation to determine the pin
# number matches with Robin's table (see doc/ directory)

for($column=0; $column<16; ++$column) {
  for($row=0; $row<16; ++$row) {
    my $pin = -1;

    # pin number (counted from 0) consists of:
    #   bit #0 if row -> pin bit #0
    my $bit0 = $row & 1;
    #  bit #2..0 of column -> pin bit #3..1
    my $bit3to1 = $column & 0x7;
    #   bit #3..1 of row -> pin bit #6..4
    my $bit6to4 = ($row & 0xe) >> 1;

    # combine to pin value
    if( $column < 8 ) {
      # left half
      if( $row >= 0 && $row <= 0x9 ) {
	$pin = $bit0 | ($bit6to4 << 4) | ($bit3to1 << 1);
      }
    } else {
      # right half
      if( $row >= 0 && $row <= 0xb ) {
	$pin = 80 + ($bit0 | ($bit6to4 << 4) | ($bit3to1 << 1));
      }
    }

    if( $pin >= 0 ) {
      printf("row:%0X  col:%0X  ->  pin:%3d\n", $row, $column, $pin+1);
    }
  }
}
