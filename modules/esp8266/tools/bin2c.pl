#!/usr/bin/perl -w
#$Id $

# reads bin files, converts to C array

my $ret = "";
my $numBytesTotal = 0;

my $bytesPerLine = 16;
my $buffer;
while( read(STDIN, $buffer, $bytesPerLine) ) {
  $ret .= "  ";
  foreach( split(//, $buffer) ) {
    $ret .= sprintf("0x%02x, ", ord($_));
    ++$numBytesTotal;
  }
  $ret .= "\n";
}

print "const u8 my_array[$numBytesTotal] = {\n${ret}};\n";
