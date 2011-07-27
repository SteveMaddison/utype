#!/usr/bin/perl -w

#
# Simple map conversion with compression by means of deduplication.
#
# (c) Copyright 2011 Steve Maddison
# 
# Input:      Saved file from "Tiled" program, XML/CSV format.
# Processing: Encodes the data into columns, replacing subsequent
#             occurrences of the same character with a meta-char
#             followed by a count. Also encodes identical subsequent
#             columns in the same way.
# Output:     C source file containing compressed map data in a
#             character array.
#

use strict;

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
	if( $line =~ /^\s*\d+,/ ) {
		my $row = $line;
		chomp( $row );

		if( $width < length($row) ) {
			$width = length($row);
		}

		$rowdata[$height] = $row;
		$height++;
	}
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
my @prev_coldata = ();
my $col_repeat = 0;

for( my $x = 0 ; $x < $width ; $x++ ) {
	# Get this column's data from each row of input.
	my @coldata = ();
	my $same = 1;
	for( my $y = 0 ; $y < $height ; $y++ ) {
		if( $rowdata[$y] =~ s/^(\d+),?// ) {
			$coldata[$y] = $1 - 1;
			if( !@prev_coldata || @prev_coldata > 0 && $coldata[$y] != $prev_coldata[$y] ) {
				$same = 0;
			}
		}
	}

	# Process/compress the column data.
	if( $same ) {
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
			my $c = $coldata[$y];

			printf "0x%02x, ", $c;
			$bytes++;
			$y++;

			# Count recurrences
			while( $y < $height && $y > 0 && $coldata[$y] == $coldata[$y-1] ) {
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

	for( my $y = 0 ; $y < $height ; $y++ ) {
		$prev_coldata[$y] = $coldata[$y];
	}
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

