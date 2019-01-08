#!/usr/bin/perl -w

# installed with "cpan Convert::Color::XYZ"
use Convert::Color::HSV;
use Convert::Color::RGB
# colours can be checked with http://colorizer.org


my @map1 = ();
my @map2 = ();
my @map3 = ();

for($i=0; $i<128; ++$i) {
  my $h = 256 - 2*$i;
  my $s = 100;
  my $v = ($i < 50) ? (2*$i) : 100;
  my $color = Convert::Color::HSV->new($h, $s / 100, $v / 100);
  my ($r, $g, $b) = $color->rgb;

  $r = 15 - int(16*$r);
  $g = 15 - int(16*$g);
  $b = 15 - int(16*$b);

  if( $r < 0 ) { $r = 0; }
  if( $g < 0 ) { $g = 0; }
  if( $b < 0 ) { $b = 0; }

  push @map1, $r;
  push @map2, $g;
  push @map3, $b;
}

print gen_map("MAP1", \@map1) . "\n";
print gen_map("MAP2", \@map2) . "\n";
print gen_map("MAP3", \@map3) . "\n";

exit;


sub gen_map
{
  my ($name, $map) = @_;

  my $out = "";

  for($i=0; $i<128; $i+=16) {
    $out .= sprintf("%-8s", ($i == 0) ? $name : "");
    for($j=0; $j<16; ++$j) {
      $out .= sprintf("%-3d ", $map->[$i + $j]);
    }
    if( $i < (128-16) ) {
      $out .= "\\";
    }
    $out .= "\n";
  }

  return $out;
}
