#!/usr/bin/perl -w
# $Id$
#########################################################################################
#
# This script converts MIDIbox SID V2 patches to C format
# The .syx file can contain a single or multiple patches.
#
# Author: tk@midibox.org (2010-01-17)
#
# SYNTAX: v2_to_c.pl <source-syx-file> <target-c-file> [<-bank n>] [<-debug>]
#
#########################################################################################

use Getopt::Long;


#########################################################################################
# Global Settings
#########################################################################################

$debug = 0;        # enabled on debug mode
$bank  = 0;        # default bank
$max_file_len = 0x80000;


#########################################################################################
# Get arguments
#########################################################################################

$command_line = "$0 " . join(" ", @ARGV);

GetOptions (
   "debug"  => \$debug,
   "bank=s" => \$bank,
   );

if( scalar(@ARGV) != 2 )
{
   die "SYNTAX:  v1_to_c.pl <source-syx-file> <target-c-file> [<-bank n>] [<-debug>]\n" .
       "EXAMPLE: v1_to_c.pl -bank 0 v2_vintage_bank.syx sid_bank_presets_a.inc\n";
}

if( $bank < 0 || $bank > 7 ) {
  die "ERROR: bank number must be in range 0..7 (A..H)\n";
}

$in_file = $ARGV[0];
$out_file = $ARGV[1];

#########################################################################################
# Read V1 Syx File
#########################################################################################

open(FILE_IN, "<${in_file}") || die "ERROR: Cannot open ${in_file} for reading!\n";
binmode(FILE_IN);
my $len = 0;
my $dump = 0;
if( ($len=sysread(FILE_IN, $dump, $max_file_len)) <= 0 )
{
   print "WARNING: ${in_file} is empty - skipping!\n";
}
elsif( $len > $max_file_len )
{
   print "WARNING: ${in_file} bigger than ${max_file_len} (definitely too large for MIOS - skipping!\n";
}
else
{
   print "Loading ${in_file} (${len} bytes)\n";
}
close(FILE_IN);

my $state = "WAIT_HEADER";
my $invalid_found = 0;
my $device_id_found = -1;
my @syx_header = (0xf0, 0x00, 0x00, 0x7e, 0x4b);
my $syx_header_pos = 0;
my $cmd = -1;
my $cmd_pos = -1;
my $checksum;
my $syx_type;
my $syx_bank;
my $syx_patch;
my $patch_byte;
my @v2_patch;
my @c_code = ();
my $patches_found = 0;

for($i=0;$i<$len;++$i) {
  my $byte = ord(substr($dump, $i, 1));

  if( $debug ) {
    printf "[%s] %02X\n", $state, $byte;
  }

  if( $byte >= 0xf8 ) {
    # ignore realtime events

  } elsif( $state ne "WAIT_HEADER" && ($byte & 0x80) ) {
    # reset state machine on any status byte (also F7...)
    $state = "WAIT_HEADER";

  } elsif( $state eq "WAIT_HEADER" ) {
    my $expected = $syx_header[$syx_header_pos];

    if( $byte != $expected ) {
      $syx_header_pos = 0;
      $invalid_found = 1;
    }

    if( $byte == $expected ) {
      ++$syx_header_pos;
      if( $syx_header_pos == scalar(@syx_header) ) {
        $syx_header_pos = 0;
        $state = "WAIT_DEVICE_ID";
      }
    }

  } elsif( $state eq "WAIT_DEVICE_ID" ) {
    if( $device_id_found == -1 ) {
      $device_id_found = $byte;
    }

    $state = "WAIT_CMD";

  } elsif( $state eq "WAIT_CMD" ) {
    $cmd = $byte;
    $cmd_pos = 0;

    if( ($cmd & 0x0f) == 0x02 ) {
      $checksum = 0;
      $syx_type=0;
      $syx_bank=0;
      $syx_patch=0;
      $state = "WRITE_CMD";
    } else {
      printf "WARNING: skipping command %02X block!\n", $cmd;
      $state = "WAIT_HEADER";
    }

  } elsif( $state eq "WRITE_CMD" ) {
    if ( $cmd_pos == 0 ) {
      $syx_type = $byte;
      if( $syx_type != 0x00 ) {
	die "ERROR: currently only syx_type 0x00 supported!\n";
      }
    } elsif( $cmd_pos == 1 ) {
      $syx_bank = $byte;
    } elsif( $cmd_pos == 2 ) {
      $syx_patch = $byte;
      $state = "PATCH_L";
      @v2_patch = ();
      $checksum = 0;
    } else {
      die "FATAL: coding error in state ${state}\n";
    }
    ++$cmd_pos;

  } elsif( $state eq "PATCH_L" ) {
    $checksum = ($checksum + $byte) & 0x7f;
    $patch_byte = $byte & 0x0f;
    $state = "PATCH_H";
  } elsif( $state eq "PATCH_H" ) {
    $checksum = ($checksum + $byte) & 0x7f;
    $patch_byte |= ($byte & 0x0f) << 4;
    push @v2_patch, $patch_byte;

    if( scalar(@v2_patch) == 512 ) {
      $state = "CHECKSUM";
    } else {
      $state = "PATCH_L";
    }
  } elsif( $state eq "CHECKSUM" ) {
    $checksum = ($checksum + $byte) & 0x7f;

    if( $checksum != 0 ) { # the sum of all checksums + the checksum itself must be zero
      printf "ERROR: checksum doesn't match - patch skipped!\n";
    } else {
      ++$patches_found;
      convert_patch($syx_type, $syx_bank, $syx_patch, @v2_patch);
    }
    $state = "F7";

  } elsif( $state eq "F7" ) {
    if( $byte != 0xf7 ) {
      die "ERROR: missing F7 - corrupted patch dump!\n";
    }

    $state = "WAIT_HEADER";

  } else {
    die "FATAL: unknown state: ${state}\n";
  }
}

if( !$patches_found ) {
  print "No patches found!\n";
  exit;
} else {
  print "${patches_found} patches found.\n";
}

#########################################################################################
# Write out C File
#########################################################################################
open(OUT, ">${out_file}") || die "ERROR: Cannot open ${out_file} for writing!\n";

printf OUT "// \$Id\$\n";
printf OUT "// converted with '${command_line}'\n";
printf OUT "\n";
printf OUT "static const u8 sid_bank_preset_${bank}[${patches_found}][512] = {\n";
foreach $line (@c_code) {
  printf OUT $line;
}
printf OUT "};\n";

print "done.\n";
exit;




#########################################################################################
#########################################################################################
## Subroutines
#########################################################################################
#########################################################################################

sub convert_patch
{
  my ($syx_type, $syx_bank, $syx_patch, @v2_patch) = @_;

  ####################################
  # print name
  ####################################
  my $name = "";
  my $i;
  for($i=0; $i<16; ++$i) {
    if( $v2_patch[$i] < 0x20 ) {
      $v2_patch[$i] = 0x20;
    }
    $name .= sprintf("%c", $v2_patch[$i]);
  }

  printf "Patch %c%03d: %s\n", ord('A')+$syx_bank, $syx_patch+1, $name;

  push @c_code, sprintf("  { // %c%03d: %s\n", ord('A')+$syx_bank, $syx_patch+1, $name);

  my $i;
  for($i=0; $i<512; $i+=16) {
    my $line = "    ";
    my $j;

    for($j=0; $j<16; ++$j) {
      $line .= sprintf("0x%02x,", $v2_patch[$i+$j]);
    }
    $line .= "\n";

    push @c_code, $line;
  }
  push @c_code, sprintf("  },\n");
}
