/*
 *  Uzebox quick and dirty tutorial
 *  Copyright (C) 2011  Steve Maddison
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <uzebox.h>

#define MAP_TILES_Y  24
#define VRAM_TILES_X 32
#define FPS          60

#include "data/level1.inc"
#include "data/scoreboard.inc"
#include "data/tiles1.inc"

#define TITLE_SECONDS   10
#define HISCORE_SECONDS	10
#define ATTRACT_SECONDS 30
#define FADE_SPEED	1

#define HIGH_SCORES 9
#define MAX_SCORE 999999999
char hi_name[HIGH_SCORES][4] = { "SAM\0","TOM\0","UZE\0","TUX\0","JIM\0","B*A\0","ABC\0","XYZ\0", "123\0" };
long hi_score[HIGH_SCORES]   = { 1000000, 900000, 800000, 700000, 600000, 500000, 400000, 300000, 200000 };

typedef enum {
	ALIGN_LEFT=0,
	ALIGN_RIGHT
} align_t;

// Globals
unsigned char *map_pos = NULL;
unsigned char *map_prev_column = NULL;
char map_col_repeat = 0;
char map_column = 0;
char scroll_speed = 0;

void map_draw_column( void ) {
	int y = 0;
	int c = 0;
	unsigned char *p = map_pos;

	if( map_col_repeat ) {
		// Copy previous column.
		p = map_prev_column;
	}
	else {
		if( *p == 0xff ) {
			// Magic...
			p++;
			if( *p == 0xff ) {
				// End of map
				scroll_speed = 0;
				return;
			}
			else {
				// Repeat previous column
				map_col_repeat = *p;
				map_pos += 2;
				p = map_prev_column;
			}
		}
	}

	// Draw the actual column.
	while( y<MAP_TILES_Y ) {
		if( *p == 0xff ) {
			// Repeat previous tile
			unsigned char t = *(p-1);
			p++;
			for( c=*p ; c>0 ; c-- ) {
				SetTile(map_column,y,t);
				y++;
			}
			p++;
		}
		else {
			SetTile(map_column,y,*p);
			p++;
			y++;
		}
	}

	if( map_col_repeat ) {
		map_col_repeat--;
	}
	else {
		map_prev_column = map_pos;
		map_pos = p;
	}

	map_column++;
	if( map_column >= VRAM_TILES_H ) {
		map_column = 0;
	}
}

void map_load( unsigned char *map_data ) {
	int x = 0;

	// Reset our map housekeeping.
	map_pos = map_data;
	map_column = 0;
	map_prev_column = NULL;
	scroll_speed = 5;

	// Draw in the first screen-full of columns.
	for( x=0 ; x<VRAM_TILES_H ; x++ ) {
		map_draw_column();
	}
}

void scroll( void ) {
	static char wait = 0;
	static char pixels = 0;

	if( scroll_speed ) {
		wait--;
		if( wait <= 0 ) {
			Scroll(1,0);
			if( ++pixels == 8 ) {
				map_draw_column();
				pixels = 0;
			}
			wait = scroll_speed;
		}
	}
}

void start_level( int level ){
	FadeOut(FADE_SPEED,true);
	SetTileTable(tiles1);
	ClearVram();
	map_load( (unsigned char*)level1_map );

	FadeIn(FADE_SPEED,false);
	while( scroll_speed ){
		WaitVsync(1);
		scroll();
	}
	WaitVsync(60);
	FadeOut(FADE_SPEED,true);
	ClearVram();
} 

void text_write( char x, char y, const char *text ) {
	char *p = (char*)text;
	char t = 0;

	while( *p ) {
		if ( *p >= 'A' && *p <= 'Z' ) {
			t = *p - 'A' + 16;
		}
		else if( *p >= '0' && *p <= '9' ) {
			t = *p - '0' + 48;
		}
		else {
			switch( *p ) {
				case '.': t=42; break;
				case '-': t=43; break;
				case '!': t=44; break;
				case '/': t=45; break;
				case '?': t=46; break;
				case '*': t=47; break;
				case 'c': t=63; break;
				default: t = 0; // blank
			}
		}
		SetTile(x++,y,t);
		p++;
	}
}

void text_write_number( char x, char y, unsigned long num, align_t align ) {
	char digits[15];
	int pos = 0;
	
	digits[pos] = 0;

	while(num > 0) {
		digits[pos] = num%10;
		num -= digits[pos];
		num /= 10;
		pos++;
	}

	if( align == ALIGN_RIGHT ) {
		x-=(pos-1);
	}

	while(--pos >= 0) {
		SetTile(x++,y,digits[pos]+48);
	}
}

void draw_starfield( void ) {
	int i;

	srandom(43);
	for( i=0 ; i<30 ; i++ ) {
		SetTile(
			random()%SCREEN_TILES_H,
			random()%SCREEN_TILES_V,
			(random()%3) + 9);
	}
}

int wait_start( int delay ) {
	int i;

	for( i=0 ; i<delay ; i++ ) {
		WaitVsync(1);
		if( ReadJoypad(0) & BTN_START ) {
			return 1;
		}
	}
	return 0;
}

int show_title() {
	int i;

	FadeOut(FADE_SPEED,true);
	ClearVram();
	draw_starfield();
	text_write((SCREEN_TILES_H-20)/2,24,"c2011 STEVE MADDISON");	
	FadeIn(FADE_SPEED,false);

	for( i=0 ; i<TITLE_SECONDS ; i++ ) {
		text_write((SCREEN_TILES_H-10)/2,20,"PUSH START");
		if( wait_start(30) ) return 1;

		text_write((SCREEN_TILES_H-10)/2,20,"          ");
		if( wait_start(30) ) return 1;
	}
	return 0;
}

int show_hi_scores() {
#define HI_SCORE_TOP 9
	int i;

	FadeOut(FADE_SPEED,true);
	ClearVram();
	draw_starfield();

	for( i=0 ; i<SCREEN_TILES_H ; i++ ) {
		SetTile(i,3,1);
		SetTile(i,22,1);
	}
	text_write((SCREEN_TILES_H-10)/2,HI_SCORE_TOP-4,"HI  SCORES");

	for( i=0 ; i<HIGH_SCORES ; i++ ) {
		text_write_number(6,HI_SCORE_TOP+i,i+1,ALIGN_RIGHT);
		SetTile(7,HI_SCORE_TOP+i, i>2 ? 15 : 12+i );
		text_write(9,HI_SCORE_TOP+i,hi_name[i]);
		text_write_number(21,HI_SCORE_TOP+i,hi_score[i],ALIGN_RIGHT);
	}

	FadeIn(FADE_SPEED,false);
	if( wait_start(HISCORE_SECONDS*FPS) ) return 1;
	return 0;
}

int show_attract() {
	return 0;
}

void show_intro() {
	//FadeOut(FADE_SPEED,true);
	//FadeIn(FADE_SPEED,true);
}

int main(){
	while(1) {
		SetScrolling(0,0);
		SetTileTable(scoreboard_tiles);
		if( show_title() || show_hi_scores() || show_attract() ) {
			// Start was pressed...
			show_intro();
			start_level(1);
		}
	}
}

