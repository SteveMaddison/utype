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

my %enemy_tile = ();
my %enemy_top_left = ();
my @enemy_list = ();
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

if( $name =~ /[12]/ ) {
	%enemy_tile = (
		# Mine
		30 => 1, 31 => 1,
		46 => 1, 47 => 1,
		62 => 1,	

		# Spinner
		68 => 1, 69 => 1, 70 => 1,
		84 => 1, 85 => 1, 86 => 1,

		71 => 1, 72 => 1, 73 => 1,
		87 => 1, 88 => 1, 89 => 1,

		74 => 1, 75 => 1, 76 => 1,
		90 => 1, 91 => 1, 92 => 1,

		77 => 1, 78 => 1, 79 => 1,
		93 => 1, 94 => 1, 95 => 1,

		# Eyeball
		64 => 1, 65 => 1, 66 => 1, 67 => 1,
		80 => 1, 81 => 1, 82 => 1, 83 => 1,
		137 => 1, 153 => 1,

		# Tentacles
		50 => 1, 51 => 1,

		# Mortar Launcher
		112 => 1, 113 => 1,
		128 => 1, 129 => 1,
		# Mortar projectiles
		114 => 1, 130 => 1,

		# Alien
		100 => 1, 101 => 1, 102 => 1,
		117 => 1, 118 => 1, 119 => 1,

		103 => 1, 104 => 1, 105 => 1,
		120 => 1, 121 => 1, 122 => 1,

		106 => 1, 107 => 1, 108 => 1,
		123 => 1, 124 => 1, 125 => 1,

		# Spike ball
		132 => 1, 133 => 1,
		148 => 1, 149 => 1,
		# Spikes
		134 => 1, 135 => 1,
		150 => 1, 151 => 1,

		# Worm
		109 => 1, 110 => 1, 111 => 1,
		125 => 1, 126 => 1, 127 => 1,

		141 => 1, 142 => 1, 143 => 1,
		157 => 1, 158 => 1, 159 => 1,

		138 => 1, 139 => 1, 140 => 1,
		154 => 1, 155 => 1, 156 => 1
	);
	%enemy_top_left = (
		30  => 'ENEMY_MINE',
		68  => 'ENEMY_SPINNER',
		71  => 'ENEMY_SPINNER',
		74  => 'ENEMY_SPINNER',
		77  => 'ENEMY_SPINNER',
		64  => 'ENEMY_EYEBALL',
		66  => 'ENEMY_EYEBALL',
		50  => 'ENEMY_TENTACLE',
		51  => 'ENEMY_TENTACLE',
		112 => 'ENEMY_MORTAR_LAUNCHER',
		100 => 'ENEMY_ALIEN',
		132 => 'ENEMY_SPIKE_BALL',
		109 => 'ENEMY_WORM'
	);
}
else {
	%enemy_tile = (
		# Hornet
		101 => 1, 102 => 1,
		117 => 1, 118 => 1,
		
		103 => 1, 104 => 1,
		119 => 1, 120 => 1,		
	);	
	%enemy_top_left = (
		101 => 'ENEMY_HORNET',
		103 => 'ENEMY_HORNET',
	);
}

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
			$coldata[$y] = $1 - 1; # Tiled IDs start at 1...

			my $enemy_id = $enemy_top_left{ $coldata[$y] };
			if( defined $enemy_id ) {
				push( @enemy_list, "$x, $y, $enemy_id" );
			}
			if( exists $enemy_tile{ $coldata[$y] } ) {
				$coldata[$y] = 0;
			}

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

print "// Enemy list:\n";
print "enemy_def_t ${name}_enemies[] PROGMEM = {\n";
foreach my $enemy_def ( @enemy_list) {
	print "\t{ $enemy_def },\n";
}
print "\t{ 0, 0, ENEMY_NONE }\n";
print "};\n\n";

