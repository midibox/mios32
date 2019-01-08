#!/usr/bin/perl -w
# $Id$
#
# Calculates capacitor charge curve and generates a gnuplot for visual check
# -> output in c_charge.inc
#
# Thorsten Klose (2007-08-05)
#


my $tau = 100.0; # exp1
#my $tau = 500.0; # exp2
my $steps = 1024;
my $y_max = 65535.0;
my $x_offset = 15.0;

####################################################################################################
open(OUT_C_DAT, ">c_charge.dat") || die;
open(OUT_C_INC, ">c_charge.inc") || die;
open(OUT_D_DAT, ">c_decharge.dat") || die;

my $scale = $y_max * ($y_max / get_exp_charge($y_max, $steps-1+$x_offset));
for($i=0; $i<$steps; ++$i) {
  my $x = $i+$x_offset;
  my $y = get_exp_charge($scale, $x);
  printf OUT_C_DAT "%d\t%d\n", $i, $y;
  printf OUT_C_INC "    %d, // %d\n", $y, $i;
  printf OUT_D_DAT "%d\t%d\n", $i, $y_max-$y;
}

close(OUT_C_DAT);
close(OUT_C_INC);
close(OUT_D_DAT);

####################################################################################################
system("gnuplot --persist -e \"plot 'c_charge.dat' title 'Charge' with lines, 'c_decharge.dat' title 'Decharge' with lines\"") && die;



####################################################################################################
sub get_exp_charge($$)
{
  my ($U, $x) = @_;
  return int($U * exp(-$tau / $x));
}
