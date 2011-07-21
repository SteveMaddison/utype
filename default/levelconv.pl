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
	'_' => 0x01,	# Ceiling
	'~' => 0x02,	# Floor
	'(' => 0x03,	# Platform (left)
	'=' => 0x04,	# Platform (middle)
	')' => 0x05,	# Platform (right)
	'<' => 0x06,	# Post (left)
	'|' => 0x07,	# Post (middle)
	'>' => 0x08	# Post (right)
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
print "const unsigned char ${name}_map[] = {\n";

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
