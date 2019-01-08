#!/usr/bin/perl
##################################################################################################
# $Id$
# This perl script generates an include file which contains the BSL code
# Usage:   perl gen_inc_file.pl <bin-file> <inc-file> <array-name>
# Example: perl gen_inc_file.pl project.bin mios32_bsl_test.inc mios32_bsl_test_image
##################################################################################################

use Fcntl;
use Getopt::Long;

my $calc_lpc17_checksum;

GetOptions (
   "calc_lpc17_checksum" => \$calc_lpc17_checksum,
   );

if( scalar(@ARGV) != 4 ) {
  die "Usage:   perl gen_inc_file.pl <bin-file> <inc-file> <array-name> <code-section> [-calc_lpc17_checksum]\n" .
      "Example: perl gen_inc_file.pl project.bin mios32_bsl_test.inc mios32_bsl_test_code mios32_bsl\n";
}

my ($bin_file, $inc_file, $array_name, $code_section) = @ARGV;
my $array_declaration = $code_section ? "__attribute__ ((section(\".${code_section}\"))) const u8" : "const u8";
my $dump_size = 0x4000; # reserved for BSL and EEPROM emulation

# read .bin file into $content array
open(IN, "<${bin_file}") || die "ERROR: '${bin_file}' does not exist!\n";
print "Reading '${bin_file}'...\n";
my $len;
my $content;
if( ($len=sysread(IN, $content, $dump_size)) <= 0 ) {
  die "ERROR: Files longer than ${dump_size} are not supported (Length: ${len} bytes)\n";
}
close(IN);

# calculate LPC17 checksum if requested
# required by boot routine, see also chapter 31.3.1.1 of LPC17 user manual ("Criterion for Valid User Code")
my $checksum = 0;
if( $calc_lpc17_checksum ) {
  for($i=0; $i<7; ++$i) {
    $checksum +=
      (ord(substr($content, 4*$i+3, 1)) << 24) |
      (ord(substr($content, 4*$i+2, 1)) << 16) |
      (ord(substr($content, 4*$i+1, 1)) <<  8) |
      (ord(substr($content, 4*$i+0, 1)) <<  0);      
  }
  $checksum = -$checksum;

  printf("Determined LPC17 Checksum: 0x%08x\n", $checksum);
}

# convert to C array
print "Writing out to '${inc_file}'\n";
open(OUT, ">${inc_file}") || die "ERROR: cannot open '${inc_file}'!\n";

printf OUT "// \$Id: \$\n";
printf OUT "// generated with '$0 " . join(" ", @ARGV) . "'\n\n";
printf OUT "${array_declaration} ${array_name}[${dump_size}] = {\n", $len;
my $line = "";
for($i=0; $i<$dump_size; ++$i) {
  my $b = ($i >= $len) ? 0xff : ord(substr($content, $i, 1));

  # insert checksum for LPC17xx
  if( $calc_lpc17_checksum ) {
    if   ( $i == (4*7+0) ) { $b = ($checksum >>  0) & 0xff; }
    elsif( $i == (4*7+1) ) { $b = ($checksum >>  8) & 0xff; }
    elsif( $i == (4*7+2) ) { $b = ($checksum >> 16) & 0xff; }
    elsif( $i == (4*7+3) ) { $b = ($checksum >> 24) & 0xff; }
  }

  $line .= sprintf("0x%02x,", $b);
  if( ($i % 16) == 15 ) {
    print OUT "$line\n";
    $line = "";
  }
}
if( length($line) ) {
  print OUT "$line\n";
}
printf OUT "};\n";

close(OUT);
