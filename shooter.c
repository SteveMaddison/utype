/*
 *  A horizontal scrolling shooter for the Uzebox
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
// Not in header...
void SetScrolling(char sx,char sy);

#define MAP_TILES_Y  24
#define FPS          60

#include "data/level1.inc"
#include "data/scoreboard.inc"
#include "data/sprites.inc"
#include "data/tiles1.inc"

const char ship_map[] PROGMEM ={
3,2
,0x1,0x0,0x0,0x9,0xa,0xb};

#define EXPLOSION_FRAMES 2
const char explosion_map_3x2[EXPLOSION_FRAMES][8] PROGMEM = {
	{ 3,2,
		16, 17, 0,
		24, 25, 16 },
	{ 3,2,
		18, 19, 0,
		26, 27, 18 }
};

// Collision detection bitmaps for tiles.
// For each tile, determine which quadrants are solid.
//  +---+---+
//  | 8 | 4 |
//  +---+---+
//  | 2 | 1 |
//  +---+---+
#define COL_LEFT    0x0a
#define COL_RIGHT   0x05
#define COL_TOP     0x0c
#define COL_BOTTOM  0x03
const unsigned char sprite_col_map[] = {
	0x00, 0x02, 0x0c, 0x02, 0x0f, 0x0f, 0x00, 0x00,
	0x00, 0x0f, 0x0e, 0x00, 0x0f, 0x0f, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const unsigned char bg_col_map[] = {
	0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x03, 0x03,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0c, 0x0c,
	0x01, 0x0f, 0x0f, 0x0e, 0x07, 0x0f, 0x0b, 0x00,
	0x07, 0x0f, 0x0f, 0x08, 0x0d, 0x0f, 0x0e, 0x00,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f
};

#define TITLE_SECONDS   10
#define HISCORE_SECONDS	10
#define ATTRACT_SECONDS 30
#define FADE_SPEED		1

typedef enum {
	ALIGN_LEFT=0,
	ALIGN_RIGHT
} align_t;

#define SHIP_MIN_X 0
#define SHIP_MAX_X ((SCREEN_TILES_H-3)*8)
#define SHIP_MIN_Y 0
#define SHIP_MAX_Y ((MAP_TILES_Y-2)*8)

typedef enum {
	STATUS_OK,
	STATUS_EXPLODING
} status_t;

typedef struct {
	int x;
	int y;
	int speed;
	status_t status;
	char anim_step;
} ship_t;
#define SPRITE_SHIP      0

typedef struct {
	int x;
	int y;
} bullet_t;
#define SPRITE_BULLET1  6
#define MAX_BULLETS     3
#define BULLET_SPEED    4
#define BULLET_DELAY   20

// Globals
ship_t ship;
bullet_t bullet[MAX_BULLETS];
unsigned char *map_pos = NULL;
unsigned char *map_prev_column = NULL;
char map_column = 0;
char map_col_repeat = 0;
char scroll_speed = 0;

#define HIGH_SCORES 9
#define MAX_SCORE 999999999
char hi_name[HIGH_SCORES][4] = { "SAM\0","TOM\0","UZE\0","TUX\0","JIM\0","B*A\0","ABC\0","XYZ\0", "123\0" };
long hi_score[HIGH_SCORES]   = { 1000000, 900000, 800000, 700000, 600000, 500000, 400000, 300000, 200000 };

void map_draw_column( void ) {
	int y = 0;
	int c = 0;
	unsigned char *p = map_pos;

	if( map_col_repeat ) {
		// Copy previous column.
		p = map_prev_column;
	}
	else {
		if( pgm_read_byte(p) == 0xff ) {
			// Magic...
			p++;
			if( pgm_read_byte(p) == 0xff ) {
				// End of map
				scroll_speed = 0;
				return;
			}
			else {
				// Repeat previous column
				map_col_repeat = pgm_read_byte(p);
				map_pos += 2;
				p = map_prev_column;
			}
		}
	}

	// Draw the actual column.
	while( y<MAP_TILES_Y ) {
		if( pgm_read_byte(p) == 0xff ) {
			// Repeat previous tile
			unsigned char t = pgm_read_byte(p-1);
			p++;
			for( c=pgm_read_byte(p) ; c>0 ; c-- ) {
				SetTile(map_column,y,t);
				y++;
			}
			p++;
		}
		else {
			SetTile(map_column,y,pgm_read_byte(p));
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
	map_col_repeat = 0;
	map_prev_column = NULL;
	scroll_speed = 5;
	ClearVram();

	// Draw in the first screen-full of columns.
	for( x=0 ; x<VRAM_TILES_H ; x++ ) {
		map_draw_column();
	}
}

void scroll( void ) {
	static char wait = 0;

	if( scroll_speed ) {
		wait--;
		if( wait <= 0 ) {
			Scroll(1,0);
			if( Screen.scrollX % 8 == 0 ) {
				map_draw_column();
			}
			wait = scroll_speed;
		}
	}
}

void clear_sprites( void ) {
	int i;
	for( i=0 ; i < MAX_SPRITES ; i++ ) {
		sprites[i].tileIndex = 0;
	}
}

int new_bullet( char tile ) {
	int i;
	for( i=0 ; i < MAX_BULLETS ; i++ ) {
		if( sprites[SPRITE_BULLET1+i].tileIndex == 0 ) {
			sprites[SPRITE_BULLET1+i].tileIndex = tile;
			bullet[i].x = ship.x + 24;
			bullet[i].y = ship.y + 10;
			return i;
		}
	}
	return -1; // No bullet slots left.
}

void update_bullet( int b ) {
	if( sprites[SPRITE_BULLET1+b].tileIndex ) {
		bullet[b].x += BULLET_SPEED;
		if( bullet[b].x >= SCREEN_TILES_H*8 ) {
			sprites[SPRITE_BULLET1+b].tileIndex = 0;
		}
		else {
			MoveSprite(SPRITE_BULLET1+b, bullet[b].x,bullet[b].y, 1,1);
		}
	}
}

int col_check( int sprite ) {
		unsigned char smap = sprite_col_map[sprites[sprite].tileIndex];

		if( smap==0 ) {
			// If all empty, no chance of collision.
			return 0;
		}
		else {
			int tile_x = (Screen.scrollX + sprites[sprite].x) / 8;
			int offset_x = (Screen.scrollX + sprites[sprite].x) % 8;
			int tile_y = sprites[sprite].y / 8;
			int offset_y = sprites[sprite].y % 8;

			// This is the tile the top-left corner of the sprite occupies.
			unsigned char *tile = &vram[0] + (tile_y * VRAM_TILES_H) + tile_x;
			
			// Build up a bitmap representing a 2x2 grid extending from the tile.
			// +-----+-----+
			// |15 14|11 10|
			// |13 12| 9  8|
			// +-----+-----+
			// | 7  6| 3  2|
			// | 5  4| 1  0|
			// +-----+-----+
			unsigned int t = (bg_col_map[(*tile)-RAM_TILES_COUNT] << 12)
						   | (bg_col_map[(*(tile+1))-RAM_TILES_COUNT] << 8);
			if( tile_y < MAP_TILES_Y )
				t |= (bg_col_map[(*(tile+VRAM_TILES_H))-RAM_TILES_COUNT] << 4)
				  | (bg_col_map[(*(tile+VRAM_TILES_H+1))-RAM_TILES_COUNT]);
			// No chance of collision if all tiles are empty.
			if( t == 0 ) {
				return 0;
			}

			// Do the same for the sprite, taking the offset into the 2x2
			// tile grid into account.
			unsigned int s = smap << 12;
			if( offset_x != 0 ) {
				// OR to the right
				s |= ((s & 0x5000) >> 3);
				s |= ((s & 0xa000) >> 1);
				if( offset_x > 3 ) {
					// Shift to two bytes right
					s = ((s & 0x0a00) >> 1)
					  | ((s & 0x5000) >> 3)
					  | ((s & 0xa000) >> 1);
				}
			}

			if( offset_y > 0 ) {
				// OR downwards
				s |= ((s & 0x3300) >> 6);
				s |= ((s & 0xcc00) >> 2);
				if( offset_y > 3 ) {
					// Shift downwards
					s = ((s & 0xcc00) >> 2)
					  | ((s & 0x3300) >> 6)
					  | ((s & 0x00cc) >> 2);
				}
			}
	
			// Now just AND the bitmasks - a collision will produce > 0;
			if( s & t ) {
				return 1;
			}
		}
		return 0;
}

void start_level( int level ){
	int i;
	unsigned int frame = 0;
	int bullet_throttle = 0;
	bool done = false;

	FadeOut(FADE_SPEED,true);
	clear_sprites();
	SetTileTable(tiles1);
	SetSpriteVisibility(true);
	SetScrolling(0,0);
	map_load( (unsigned char*)level1_map );

	MapSprite(0, ship_map);
	ship.x = 16;
	ship.y = SHIP_MAX_Y/2;
	ship.speed = 1;
	ship.status = STATUS_OK;

	FadeIn(FADE_SPEED,false);
	while( !done ) {
		unsigned int buttons = ReadJoypad(0);

		WaitVsync(1);
		scroll();
		
		if( ship.status == STATUS_OK ) {
			if( buttons & BTN_LEFT ) {
				ship.x -= ship.speed;
				if( ship.x < SHIP_MIN_X ) ship.x = SHIP_MIN_X;
			}
			else if( buttons & BTN_RIGHT ) {
				ship.x += ship.speed;
				if( ship.x > SHIP_MAX_X ) ship.x = SHIP_MAX_X;
			}
			if( buttons & BTN_UP ) {
				ship.y -= ship.speed;
				if( ship.y < SHIP_MIN_Y ) ship.y = SHIP_MIN_Y;
			}
			else if( buttons & BTN_DOWN ) {
				ship.y += ship.speed;
				if( ship.y > SHIP_MAX_Y ) ship.y = SHIP_MAX_Y;
			}
			if( buttons & BTN_B ) {
				if( bullet_throttle == 0 ) {
					new_bullet(2);
					bullet_throttle = BULLET_DELAY;
				}
			}
			MoveSprite(0, ship.x,ship.y, 3,2);
		}
		else if( ship.status == STATUS_EXPLODING ) {
			if( ship.anim_step == 16 )
				MapSprite( 0, explosion_map_3x2[0] );

			if( ship.anim_step == 8 )
				MapSprite( 0, explosion_map_3x2[1] );
			
			if( ship.anim_step == 0 ) {
				for( i=0 ; i<SPRITE_BULLET1 ; i++ ) {
					sprites[i].tileIndex = 0;
				}
				done = 1;
			}
			ship.anim_step--;
		}

		for( i=0 ; i<MAX_BULLETS ; i++ ) {
			update_bullet(i);
		}

		// Collison detection
		for( i=0 ; i<MAX_SPRITES ; i++ ) {
			if( sprites[i].tileIndex ) {
				if( col_check(i) ) {
					if( i<SPRITE_BULLET1 ) {
						// Ship has crashed
						if( ship.status != STATUS_EXPLODING ) {
							ship.status = STATUS_EXPLODING;
							ship.anim_step = 16;
						}
					}
					else if( i < SPRITE_BULLET1+MAX_BULLETS ) {
						// Bullet hit something...
						sprites[i].tileIndex = 0;
					}
				}
			}
		}

		frame++;
		if( bullet_throttle ) bullet_throttle--;
	}
	WaitVsync(60);
	FadeOut(FADE_SPEED,true);
	SetSpriteVisibility(false);
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
		if( wait_start(FPS/2) ) return 1;

		text_write((SCREEN_TILES_H-10)/2,20,"          ");
		if( wait_start(FPS/2) ) return 1;
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
	SetSpritesTileTable(sprite_tiles);
	while(1) {
		SetScrolling(0,0);
		SetTileTable(scoreboard_tiles);
		//if( show_title() || show_attract() || show_hi_scores() ) {
			// Start was pressed...
			show_intro();
			start_level(1);
		//}
	}
}

