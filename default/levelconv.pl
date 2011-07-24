#!/usr/bin/perl -w

#
# Simple map conversion with compression by means of deduplication.
#
# (c) Copyright 2011 Steve Maddison
# 
# Input:      Plain text file containing map drawn in ASCII art.
#             Lines must be prefixed with digits (i.e. line numbers).
# Processing: Encodes the data into columns, replacing subsequent
#             occurrences of the same character with a meta-char
#             followed by a count. Also encodes identical subsequent
#             columns in the same way.
# Output:     C source file containing compressed map data in a
#             character array.
#

use strict;

# Map ASCII characters in the input to tile numbers in the output.
my %charmap = (
	' ' => 0x00,	# Blank
	'.' => 0x01,	# Alt. BG
	'+' => 0x02,	# Alt. BG
	'*' => 0x03,	# Alt. BG
	'{' => 0x04,	# Alt. BG
	'-' => 0x05,	# Alt. BG
	'O' => 0x06,	# Alt. BG
	'@' => 0x07,	# Alt. BG
	'_' => 0x08,	# Ceiling
	'~' => 0x09,	# Floor
	'(' => 0x0a,	# Platform (left)
	'=' => 0x0b,	# Platform (middle)
	')' => 0x0c,	# Platform (right)
	'<' => 0x0d,	# Post (left)
	'|' => 0x0e,	# Post (middle)
	'>' => 0x0f,	# Post (right)
	'a' => 0x20,
	'b' => 0x21,
	'c' => 0x22,
	'd' => 0x23,
	'e' => 0x24,
	'f' => 0x25,
	'g' => 0x26,
	'h' => 0x27,
	'i' => 0x28,
	'j' => 0x29,
	'k' => 0x2a,
	'l' => 0x2b,
	'm' => 0x2c,
	'n' => 0x2d,
	'o' => 0x2e,
	'p' => 0x2f,
	'q' => 0x30,
	'r' => 0x31,
	's' => 0x32,
	't' => 0x33,
	'u' => 0x34,
	'v' => 0x35,
	'w' => 0x36,
	'x' => 0x37,
	'y' => 0x38,
	'z' => 0x39,
	'A' => 0x3a,
	'B' => 0x3b,
	'C' => 0x3c,
	'D' => 0x3d,
	'E' => 0x3e,
	'F' => 0x3f
);

my $width = 0;
my $height = 0;
my @rowdata;
my $bytes = 0;

my $name = $ARGV[0] || '';

if( $name eq '' ) {
	die "No file name provided\n";
}

$name =~ s/^.*\///;
$name =~ s/\..*$//;

# Read in the whole file;
while( my $line = <> ) {
	if( $line =~ /^\d+(.*)$/ ) {
		my $row = $1;
		chomp( $row );

		if( $width < length($row) ) {
			$width = length($row);
		}

		$rowdata[$height] = $row;
		$height++;
	}
}

for( my $y = 0 ; $y < $height ; $y++ ) {
	# Pad short lines with spaces (this saves checking later).
	$rowdata[$y] .= ' ' x ($width - length($rowdata[$y]));
}

print "//\n";
print "// Generated map data for '$name'\n";
print "// Dimensions: $width x $height\n";
print "//\n";
print "\n";
print "#define REPEAT(x) 0xff, x\n";
print "#define " . uc($name) . "_MAP_WIDTH  $width\n";
print "#define " . uc($name) . "_MAP_HEIGHT $height\n";
print "\n";
print "const unsigned char ${name}_map[] PROGMEM = {\n";

# Keep track of any subsequent identical columns.
my $prev_coldata = '';
my $col_repeat = 0;

for( my $x = 0 ; $x < $width ; $x++ ) {
	# Get this column's data from each row of input.
	my $coldata = '';
	for( my $y = 0 ; $y < $height ; $y++ ) {
		my $c = substr($rowdata[$y],$x,1);
		if( exists $charmap{ $c } ) {
			$coldata .= $c;
		}
		else {
			warn "$name: Character '$c' at ($x,$y) not recognised - using blank instead.\n";
			$coldata .= ' ';
		}
	}

	# Process/compress the column data.
	if( $coldata eq $prev_coldata ) {
		$col_repeat++;
	}
	else {
		if( $col_repeat ) {
			print "\tREPEAT($col_repeat),\n";
			$bytes += 2;
			$col_repeat = 0;
		}

		my $y = 0;
		print "\t";
		do {
			my $repeat = 0;
			my $c = $charmap{ substr($coldata,$y,1) };

			printf "0x%02x, ", $c;
			$bytes++;
			$y++;

			# Count recurrences
			while( $y < $height && substr($coldata,$y,1) eq substr($coldata,$y-1,1) ) {
				$repeat++;
				$y++;
			}

			if( $repeat ) {
				if( $repeat <= 2 ) {
					while( $repeat-- ) {
						printf "0x%02x, ", $c;
						$bytes++;
					}
				}
				else {
					print  "REPEAT($repeat), ";
					$bytes += 2;
				}
			}
		} while ( $y < $height );

		print "// Column $x\n";
	}

	$prev_coldata = $coldata;
}

print "\t0xff, 0xff // Terminator\n";
$bytes += 2;

print "};\n";
print "\n";

print "// STATISTICS:\n";
print "// Original map size   = ", $width * $height, " bytes\n";
print "// Compressed map size = ", $bytes, " bytes\n";
print "// Compression ratio   = ", int( ($bytes/($width*$height)*100) + 0.5 ), "%\n";
print "\n";

