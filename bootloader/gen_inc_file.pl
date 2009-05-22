#!/usr/bin/perl
##################################################################################################
# $Id$
# This perl script generates an include file which contains the BSL code
# Usage:   perl gen_inc_file.pl <bin-file> <inc-file> <array-name>
# Example: perl gen_inc_file.pl project.bin mios32_bsl_test.inc mios32_bsl_test_image
##################################################################################################

use Fcntl;

if( scalar(@ARGV) != 4 ) {
  die "Usage:   perl gen_inc_file.pl <bin-file> <inc-file> <array-name> <code-section>\n" .
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

# convert to C array
print "Writing out to '${inc_file}'\n";
open(OUT, ">${inc_file}") || die "ERROR: cannot open '${inc_file}'!\n";

printf OUT "// \$Id: \$\n";
printf OUT "// generated with '$0 " . join(" ", @ARGV) . "'\n\n";
printf OUT "${array_declaration} ${array_name}[${dump_size}] = {\n", $len;
my $line = "";
for($i=0; $i<$dump_size; ++$i) {
  my $b = ($i >= $len) ? 0xff : ord(substr($content, $i, 1));
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
