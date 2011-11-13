#!/usr/bin/perl
# generates a bank file
# usage example: perl gen_bank.pl *.raw > bank.1

my $key = 0x24;
while( @ARGV && $key < 0x80 ) {
  $file = shift @ARGV;
  printf "0x%02x %s\n", $key, $file;
  ++$key;
}
