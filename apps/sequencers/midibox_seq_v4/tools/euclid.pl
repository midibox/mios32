# $Id$
# checking the algorithm from http://crx091081gb.net/?p=189

sub eugen($$)
{
  my ($steps, $pulses) = @_;
  my $ret = "";

  if( $pulses >= $steps ) {
    for($i=0; $i<$steps; ++$i) {
      $ret .= "*";
    }
  } elsif( $pulses == 0 ) {
    for($i=0; $i<$steps; ++$i) {
      $ret .= ".";
    }
  } elsif( $steps == 1 ) {
    $ret = $pulses ? "*" : ".";
  } else {
    my $pauses = $steps - $pulses;

    if( $pauses >= $pulses ) { # first case: more pauses than pulses
      my $per_pulse = $pauses / $pulses;
      my $remainder = $pauses % $pulses;
      for($i=0; $i<$pulses; ++$i) {
	$ret .= "*";
	for($j=0; $j<$per_pulse; ++$j) {
	  $ret .= ".";
	}
	if( $i < $remainder ) {
	  $ret .= ".";
	}
      }
    } else { # second case: more pulses than pauses
      my $per_pause = ($pulses - $pauses) / $pauses;
      my $remainder = ($pulses - $pauses) % $pauses;

      for($i=0; $i<$pauses; ++$i) {
	$ret .= "*";
	$ret .= ".";
	for($j=0; $j<$per_pause; ++$j) {
	  $ret .= "*";
	}
	if( $i < $remainder ) {
	  $ret .= "*";
	}
      }
    }
  }

  return $ret;
}

my $steps = 16;
for($pulses=1; $pulses<=16; ++$pulses) {
  printf "%2d/%2d   %s\n", $steps, $pulses, eugen($steps, $pulses);
}
