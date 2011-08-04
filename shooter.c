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

#define LEVEL_TILES_Y  24
#define FPS            60

#include "data/tiles1.inc"
#include "data/overlay.inc"
#include "data/tiles2.inc"
#include "data/sprites.inc"
#include "data/sfx.inc"

typedef enum {
	ENEMY_NONE,
	ENEMY_MINE,
	ENEMY_MORTAR_LAUNCHER,
	ENEMY_MORTAR,
	ENEMY_SPINNER,
	ENEMY_EYEBALL,
	ENEMY_TENTACLE,

	ENEMY_EXP_2X2,
	ENEMY_EXP_3X2,
	ENEMY_COUNT
} enemy_id_t;

#define MAX_ENEMIES 15
#define HP_INFINITE -1
char enemy_hp[ENEMY_COUNT] PROGMEM = {
	0,
	5,				// Mine
	3,				// Mortar Launcher
	HP_INFINITE,	// Mortar
	3,				// Spinner
	16,				// Eyeball
	HP_INFINITE		// Tentacle
};

int enemy_score[ENEMY_COUNT] PROGMEM = {
	0,
	1000,	// Mine
	1250,	// Mortar Launcher
	0,
	200,	// Spinner
	2500,	// Eyeball
	0		// Tentacle
};

typedef struct {
	char x;
	char y;
	enemy_id_t id;
} enemy_def_t;

typedef struct {
	char x;
	char y;
	enemy_id_t id;
	char hp;
	int anim_step;
	unsigned char whooshed;
} enemy_t;

#include "data/level1.inc"
#include "data/level2.inc"

#define OVERLAY_OFFSET(x) ((x<3) ? (192-RAM_TILES_COUNT) : 0)

#define EXPLOSION_FRAMES_S 2
const char ship_explosion_map[EXPLOSION_FRAMES_S][6] PROGMEM = {
	{ 2,2,	20, 28,
			29, 21 },
	{ 2,2,	22, 30,
			31, 23 }
};
const char explosion_map_2x2[EXPLOSION_FRAMES_S][6] PROGMEM = {
	{ 2,2,	144, 145,
			160, 161 },
	{ 2,2,	146, 147,
			162, 163 }
};

#define EXPLOSION_FRAMES_M 3
const char explosion_map_3x2[EXPLOSION_FRAMES_M][8] PROGMEM = {
	{ 3,2,	144, 145, 0,
			160, 161, 0 },
	{ 3,2,	146, 144, 145,
			162, 160, 161 },
	{ 3,2,	147, 146, 147,
			163, 162, 163 }
};

#define WHOOSH_FRAMES 2
const char whoosh_map[] PROGMEM = {
	4,2,	32, 33, 34, 35,
			40, 41, 42, 43
};

char mine_map[2][6] PROGMEM = {
	{ 2,2,	30,31,
			46,47 },
	{ 2,2,	30,31,
			62,47 }
};

char spinner_map[4][8] PROGMEM = {
	{ 3,2,	68,69,70,
			84,85,86 },
	{ 3,2,	71,72,73,
			87,88,89 },
	{ 3,2,	74,75,76,
			90,91,92 },
	{ 3,2,	77,78,79,
			93,94,95 }
};

char eyeball_map[2][6] PROGMEM = {
	{ 2,2,	66,67,
			82,83 },
	{ 2,2,	64,65,
			80,81 }
};
char dead_eyeball_map[10] PROGMEM = {
	2,4,	96,0,
			0,66,
			0,82,
			48,0
};

char tentacle_map[2][6] PROGMEM = {
	{ 4,1,	50,51,50,51 },
	{ 4,1,	51,50,51,50 }
};

char mortar_map[6] PROGMEM = {
	2,2,	112,113,
			128,129
};
#define MORTAR_TL	114
#define MORTAR_BR	130

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
const unsigned char sprite_col_map[] PROGMEM = {
	0x00, 0x02, 0x0c, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
	0x00, 0x0f, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x02, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0f, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f
};
const unsigned char bg_col_map[] PROGMEM = {
	0x00, 0x07, 0x0b, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f,   0x07, 0x0f, 0x07, 0x0b, 0x0f, 0x0f, 0x06, 0x09,
	0x01, 0x0f, 0x0e, 0x07, 0x0f, 0x0b, 0x0f, 0x0f,   0x03, 0x03, 0x03, 0x07, 0x0b, 0x0f, 0x07, 0x0b,
	0x07, 0x0f, 0x02, 0x0d, 0x0f, 0x0e, 0x0f, 0x0f,   0x0f, 0x0f, 0x0f, 0x0d, 0x0e, 0x0f, 0x0d, 0x0e,
	0x05, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x04, 0x08,   0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x0d, 0x0f,

	0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x03, 0x0f, 0x01,   0x07, 0x0f, 0x05, 0x07, 0x0a, 0x03, 0x0f, 0x00,
	0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x0c, 0x0f, 0x0f,   0x0d, 0x0f, 0x05, 0x0f, 0x0a, 0x0c, 0x0f, 0x00,
	0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x0f, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x05, 0x0f, 0x01, 0x0f, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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
#define SHIP_MAX_Y (((LEVEL_TILES_Y-2)*8)-4)

typedef enum {
	STATUS_OK,
	STATUS_EXPLODING
} status_t;

typedef struct {
	int x;
	int y;
	int speed;
	status_t status;
	int anim_step;
} ship_t;
#define SPRITE_SHIP      0

typedef enum {
	BULLET_FREE = 0,
	BULLET_CHARGING,
	BULLET_SMALL,
	BULLET_MEDIUM,
	BULLET_LARGE
} bullet_status_t;

typedef struct {
	int x;
	int y;
	bullet_status_t status;
} bullet_t;
#define SPRITE_BULLET1  4
#define MAX_BULLETS     6
#define BULLET_SPEED    4
#define BULLET_DELAY    ((SCREEN_TILES_H*8)/BULLET_SPEED/(MAX_BULLETS-1))
#define BULLET_CHARGE_MAX (FPS)
#define SPRITE_WHOOSH (SPRITE_BULLET1+MAX_BULLETS)
#define WHOOSH_SPRITES  8

#define MAX_LIVES 9

// Globals
unsigned int frame = 0;
ship_t ship;
bullet_t bullet[MAX_BULLETS];
char bullet_charge = 0;
char level = 0;
long score = 0;
char lives = 0;
unsigned char *level_pos = NULL;
unsigned char *level_prev_column = NULL;
char level_vram_column = 0;
char level_col_repeat = 0;
int level_column = 0;
char scroll_speed = 0;
char scroll_countdown = 0;
enemy_def_t *enemy_pos = NULL;
enemy_t enemies[MAX_ENEMIES];

#define HIGH_SCORES 8
#define MAX_SCORE 999999999
char hi_name[HIGH_SCORES][4] = { "SAM\0","TOM\0","UZE\0","TUX\0","JIM\0","B*A\0","ABC\0","XYZ\0" };
long hi_score[HIGH_SCORES]   = { 1000000, 900000, 800000, 700000, 600000, 500000, 400000, 300000 };

int add_enemy( enemy_id_t id, int x, int y ) {
	int i;
	for( i=0 ; i < MAX_ENEMIES ; i++ ) {
		if( enemies[i].id == ENEMY_NONE ) {
			enemies[i].id = id;
			enemies[i].x = x;
			enemies[i].y = y;
			enemies[i].hp = pgm_read_byte( &enemy_hp[id] );
			enemies[i].whooshed = 0;
			enemies[i].anim_step = 0;
			return i;
		}
	}
	return -1;
}

void level_draw_column( void ) {
	int y = 0;
	int c = 0;
	unsigned char *p = level_pos;

	if( scroll_countdown )
		return;

	if( level_col_repeat ) {
		// Copy previous column.
		p = level_prev_column;
	}
	else {
		if( pgm_read_byte(p) == 0xff ) {
			// Magic...
			p++;
			if( pgm_read_byte(p) == 0xff ) {
				// End of level
				scroll_countdown = 24;
				while( pgm_read_byte( &enemy_pos->id ) != ENEMY_NONE ) {
					while( pgm_read_byte( &enemy_pos->x ) == level_column-3 ) {
							add_enemy( pgm_read_byte( &enemy_pos->id ), level_vram_column-3, pgm_read_byte( &enemy_pos->y ) );
							enemy_pos++;
					}
					level_column++;
					level_vram_column++;
				}
				return;
			}
			else {
				// Repeat previous column
				level_col_repeat = pgm_read_byte(p);
				level_pos += 2;
				p = level_prev_column;
			}
		}
	}

	// Draw the actual column.
	while( y<LEVEL_TILES_Y ) {
		if( pgm_read_byte(p) == 0xff ) {
			// Repeat previous tile
			unsigned char t = pgm_read_byte(p-1);
			p++;
			for( c=pgm_read_byte(p) ; c>0 ; c-- ) {
				SetTile(level_vram_column,y,t);
				y++;
			}
			p++;
		}
		else {
			SetTile(level_vram_column,y,pgm_read_byte(p));
			p++;
			y++;
		}
	}

	// Any new enemies?
	while( pgm_read_byte( &enemy_pos->x ) == level_column-3 ) {
		add_enemy( pgm_read_byte( &enemy_pos->id ), level_vram_column-3, pgm_read_byte( &enemy_pos->y ) );
		enemy_pos++;
	}

	if( level_col_repeat ) {
		level_col_repeat--;
	}
	else {
		level_prev_column = level_pos;
		level_pos = p;
	}

	level_column++;	
	level_vram_column++;
	if( level_vram_column >= VRAM_TILES_H ) {
		level_vram_column = 0;
	}
}

void level_load( unsigned char *level_data ) {
	int x = 0;

	// Reset our level housekeeping.
	level_pos = level_data;
	level_vram_column = 0;
	level_column = 0;
	level_col_repeat = 0;
	level_prev_column = NULL;
	scroll_countdown = 0;
	scroll_speed = 5;
	ClearVram();

	// Draw in the first screen-full of columns.
	for( x=0 ; x<VRAM_TILES_H ; x++ ) {
		level_draw_column();
	}
}

void scroll( void ) {
	static char wait = 0;

	if( scroll_speed ) {
		wait--;
		if( wait <= 0 ) {
			Scroll(1,0);
			if( Screen.scrollX % 8 == 0 ) {
				level_draw_column();
			}
			wait = scroll_speed;
			if( scroll_countdown ) {
				scroll_countdown--;
				if( scroll_countdown == 0 ) {
					scroll_speed = 0;
				}
			}
		}
	}
}

void clear_sprites( void ) {
	int i;
	for( i=0 ; i < MAX_SPRITES ; i++ ) {
		sprites[i].tileIndex = 0;
	}
}

void set_bullet( int b, bullet_status_t status ) {
	switch( status ) {
		case BULLET_FREE:
			sprites[SPRITE_BULLET1+b].tileIndex = 0;
			if( bullet[b].status == BULLET_LARGE ) {
				int i;
				for( i=0 ; i<8 ; i++ ) {
					sprites[SPRITE_WHOOSH+i].tileIndex = 0;
				}
			}
			break;
		case BULLET_CHARGING:
			sprites[SPRITE_BULLET1+b].tileIndex = 3;
			break;
		case BULLET_SMALL:
			TriggerFx( SFX_FIRE, 0xff, true );
			sprites[SPRITE_BULLET1+b].tileIndex = 2;
			bullet[b].y++; // offset for small bullet sprite
			bullet[b].y &= 0xfffe;
			break;
		case BULLET_MEDIUM:
			TriggerFx( SFX_FIRE, 0xff, true );
			sprites[SPRITE_BULLET1+b].tileIndex = 6;
			bullet[b].y &= 0xfffe;
			break;
		case BULLET_LARGE:
			TriggerFx( SFX_WHOOSH, 0xff, true );
			bullet[b].y -= 4;
			MapSprite(SPRITE_WHOOSH, whoosh_map);
			sprites[SPRITE_BULLET1+b].tileIndex = 0;
			break;
		default:
			break;
	}
	bullet[b].status = status;
}

int new_bullet( void ) {
	int i;
	for( i=0 ; i < MAX_BULLETS ; i++ ) {
		if( bullet[i].status == BULLET_FREE ) {
			set_bullet( i, BULLET_CHARGING );
			bullet[i].x = 0;
			bullet[i].y = 0;
			return i;
		}
	}
	return -1; // No bullet slots left.
}

void update_bullet( int b ) {
	if( bullet[b].status != BULLET_FREE ) {
		if( bullet[b].x >= SCREEN_TILES_H*8 ) {
			set_bullet(b, BULLET_FREE);
			if( bullet[b].status == BULLET_LARGE ) {
				for( int i=0 ; i<MAX_ENEMIES ; i++ ) {
					enemies[i].whooshed = 0;
				}
			}
		}
		else {
			switch( bullet[b].status ) {
				case BULLET_CHARGING:
					bullet[b].x = ship.x + 21;
					bullet[b].y = ship.y + 9;
					if( bullet_charge > BULLET_CHARGE_MAX/8 ) {
						if( frame % 4 == 0 ) {
							sprites[SPRITE_BULLET1+b].tileIndex++;
							if( sprites[SPRITE_BULLET1+b].tileIndex > 5 ) {
								sprites[SPRITE_BULLET1+b].tileIndex = 3;
							}
						}
					}
					MoveSprite(SPRITE_BULLET1+b, bullet[b].x,bullet[b].y, 1,1);
					break;
				case BULLET_SMALL:
				case BULLET_MEDIUM:
					bullet[b].x += BULLET_SPEED;
					MoveSprite(SPRITE_BULLET1+b, bullet[b].x,bullet[b].y, 1,1);
					break;
				case BULLET_LARGE:
					if( bullet[b].x >= (SCREEN_TILES_H*8)-16 ) {
						// Prevent wrapping
						sprites[SPRITE_WHOOSH+2].tileIndex = 0;
						sprites[SPRITE_WHOOSH+3].tileIndex = 0;
						sprites[SPRITE_WHOOSH+6].tileIndex = 0;
						sprites[SPRITE_WHOOSH+7].tileIndex = 0;
					}
					bullet[b].x += BULLET_SPEED;
					MoveSprite(SPRITE_WHOOSH, bullet[b].x,bullet[b].y, 4,2);
					break;
				default:
					break;
			}
		}
	}
}

void fill_tiles( int x, int y, int width, int height, unsigned char t ) {
	int xx,yy,rx;
/*
	for( yy=0 ; yy<height ; yy++ ) {	
		for( xx=0 ; xx<width ; xx++ ) {
			vram[((y+yy)*VRAM_TILES_H)+((xx+x)%VRAM_TILES_H)]=t+RAM_TILES_COUNT;
		}
	}
*/

	for( xx=0 ; xx<width ; xx++ ) {
		rx = (x+xx)%VRAM_TILES_H;
		if( rx == 0 && xx > 0 ) {
			y--;
		}
		for( yy=0 ; yy<height ; yy++ ) {
			vram[((y+yy)*VRAM_TILES_H)+rx]=t+RAM_TILES_COUNT;
		}
	}
}

void clear_enemy( int e ) {
	switch( enemies[e].id ) {
		case ENEMY_MINE:
		case ENEMY_EXP_2X2:
			fill_tiles( enemies[e].x, enemies[e].y, 2, 2, 0 );
			break;
		case ENEMY_SPINNER:
		case ENEMY_EXP_3X2:
			fill_tiles( enemies[e].x, enemies[e].y, 3, 2, 0 );
			break;
		default:
			break;
	}
	enemies[e].x = level_vram_column;
	enemies[e].y = 0;
	enemies[e].id = ENEMY_NONE;
}

void clear_enemies() {
	int i;
	
	for( i=0 ; i<MAX_ENEMIES ; i++ ) {
		clear_enemy(i);
	}
}

void draw_enemy( char x, char y, const char *map ) {
// This is basically DrawMap2() with wrapping and transparency.
	unsigned char i;
	unsigned char mapWidth=pgm_read_byte(&(map[0]));
	unsigned char mapHeight=pgm_read_byte(&(map[1]));

	for(int dy=0;dy<mapHeight;dy++){
		for(int dx=0;dx<mapWidth;dx++){			
			if(( i=pgm_read_byte(&(map[(dy*mapWidth)+dx+2])) ))
				vram[((y+dy)*VRAM_TILES_H)+((x+dx)%VRAM_TILES_H)]=(i + RAM_TILES_COUNT) ;
		}
	}
}

void update_enemies() {
	int i;

	for( i=0 ; i<MAX_ENEMIES ; i++ ) {
		if( enemies[i].x < 0 ) {
			enemies[i].x = VRAM_TILES_H - 1;
		}
		if( enemies[i].x == level_vram_column-3 ) {
			clear_enemy( i );
		}
		else {
			switch( enemies[i].id ) {
				case ENEMY_NONE:
					break;
				case ENEMY_MINE:
					switch( enemies[i].anim_step % 60 ) {
						case 0:
							draw_enemy( enemies[i].x, enemies[i].y, mine_map[0] );
							break;
						case 30:
							draw_enemy( enemies[i].x, enemies[i].y, mine_map[1] );
							break;
						default:
							break;
					}
					break;
				case ENEMY_MORTAR_LAUNCHER:
					if( enemies[i].anim_step % 90 == 0 ) {
						draw_enemy( enemies[i].x, enemies[i].y, mortar_map );
						add_enemy( ENEMY_MORTAR, enemies[i].x-1, enemies[i].y-1 );
					}
					break;
				case ENEMY_MORTAR:
					switch( enemies[i].anim_step % 8 ) {
						case 0:
							SetTile( enemies[i].x, enemies[i].y, 0 );
							if( --enemies[i].y < 0 || enemies[i].x == level_vram_column ) {
								clear_enemy(i);
							}
							else {
								if( --enemies[i].x < 0 ) {
									enemies[i].x = VRAM_TILES_H - 1;
								}
								SetTile( enemies[i].x, enemies[i].y, MORTAR_BR );
							}
							break;
						case 4:
							SetTile( enemies[i].x, enemies[i].y, MORTAR_TL );
							break;
					}
					break;
				case ENEMY_SPINNER:
					switch( enemies[i].anim_step % 16 ) {
						case 0:
							fill_tiles( enemies[i].x+2, enemies[i].y, 1, 2, 0 );
							enemies[i].x--;
							if( enemies[i].x < 0 ) {
								enemies[i].x = VRAM_TILES_H - 1;
							}
							draw_enemy( enemies[i].x, enemies[i].y, spinner_map[0] );
							break;
						case 4:
							draw_enemy( enemies[i].x, enemies[i].y, spinner_map[1] );
							break;
						case 8:
							draw_enemy( enemies[i].x, enemies[i].y, spinner_map[2] );
							break;
						case 12:
							draw_enemy( enemies[i].x, enemies[i].y, spinner_map[3] );
							break;
						default:
							break;
					}
					break;
				case ENEMY_EYEBALL:
					switch( enemies[i].anim_step ) {
						case 0:
							// Closed, set timer.
							draw_enemy( enemies[i].x, enemies[i].y, eyeball_map[0] );
							enemies[i].anim_step = (random()%120) - (i%4)*20;
							break;
						case 420:
							// Open eye.
							draw_enemy( enemies[i].x, enemies[i].y, eyeball_map[1] );
							break;
						case 540:
						case 550:
						case 560:
						case 570:
						case 580:
						case 590:
						case 600:
						case 610:
						case 620:
						case 630:
						case 640:
						case 650:
							// Fire!
							fill_tiles( enemies[i].x-VRAM_TILES_H+5, enemies[i].y+1, VRAM_TILES_H-5, 1, 52 );
							fill_tiles( enemies[i].x-VRAM_TILES_H+5, enemies[i].y+2, VRAM_TILES_H-5, 1, 53 );
							if( Screen.scrollY == 0 ) Scroll( 0, -2 );
							break;
						case 545:
						case 555:
						case 565:
						case 575:
						case 585:
						case 595:
						case 605:
						case 615:
						case 625:
						case 635:
						case 645:
						case 655:
							// Beam flash
							fill_tiles( enemies[i].x-VRAM_TILES_H+5, enemies[i].y+1, VRAM_TILES_H-5, 1, 53 );
							fill_tiles( enemies[i].x-VRAM_TILES_H+5, enemies[i].y+2, VRAM_TILES_H-5, 1, 52 );
							SetScrolling( Screen.scrollX, 0 );
							break;
						case 660:
							// Stop firing
							fill_tiles( enemies[i].x-VRAM_TILES_H+5, enemies[i].y+1, VRAM_TILES_H-5, 2, 0 );
							break;
						case 780:
							// Loop
							//draw_enemy( enemies[i].x, enemies[i].y, eyeball_map[0] );
							enemies[i].anim_step = -1;
							break;
					}
					break;
				case ENEMY_TENTACLE:
					switch( enemies[i].anim_step % 40 ) {
						case 0:
							draw_enemy( enemies[i].x, enemies[i].y, tentacle_map[0] );
							enemies[i].anim_step = random()%12;
							break;
						case 20:
							draw_enemy( enemies[i].x, enemies[i].y, tentacle_map[1] );
							break;
					}
					break;
				case ENEMY_EXP_2X2:
					switch( enemies[i].anim_step % 30 ) {
						case 0:
							draw_enemy( enemies[i].x, enemies[i].y, explosion_map_2x2[0] );
							break;
						case 10:
							draw_enemy( enemies[i].x, enemies[i].y, explosion_map_2x2[1] );
							break;
						case 20:
							fill_tiles( enemies[i].x, enemies[i].y, 2, 2, 0 );
							clear_enemy(i);
							break;
						default:
							break;
					}
					break;
				case ENEMY_EXP_3X2:
					switch( enemies[i].anim_step % 30 ) {
						case 0:
							draw_enemy( enemies[i].x, enemies[i].y, explosion_map_3x2[0] );
							break;
						case 5:
							draw_enemy( enemies[i].x, enemies[i].y, explosion_map_3x2[1] );
							break;
						case 10:
							draw_enemy( enemies[i].x, enemies[i].y, explosion_map_3x2[2] );
							break;
						case 15:
							fill_tiles( enemies[i].x, enemies[i].y, 3, 2, 0 );
							clear_enemy(i);
							break;
					}
					break;
				default:
					break;
			}
			enemies[i].anim_step++;
		}
	}
}

void text_write( char x, char y, const char *text, bool overlay ) {
	char *p = (char*)text;
	unsigned char t = 0;

	while( *p ) {
		if ( *p >= 'A' && *p <= 'Z' ) {
			t = *p - 'A' + 1;
		}
		else if( *p >= '0' && *p <= '9' ) {
			t = *p - '0' + 32;
		}
		else {
			switch( *p ) {
				case '.': t=27; break;
				case '!': t=28; break;
				case '/': t=29; break;
				case '?': t=30; break;
				case '*': t=31; break;
				case 'c': t=44; break;
				default: t = 0; // blank
			}
		}
		SetTile(x++, y, t + OVERLAY_OFFSET(level) + (overlay ? RAM_TILES_COUNT : 0 ) );
		p++;
	}
}

void text_write_number( char x, char y, unsigned long num, align_t align, bool overlay ) {
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

	if(pos == 0) {
		if( align == ALIGN_RIGHT )
			x--;
		pos++;
	}
	while(--pos >= 0) {
		if( overlay ) {
			vram[(VRAM_TILES_H*(VRAM_TILES_V+y))+x] = digits[pos] + 32 + OVERLAY_OFFSET(level) + RAM_TILES_COUNT;
			x++;
		}
		else {
			SetTile(x++,y,digits[pos] + 32 + OVERLAY_OFFSET(level) );
		}
	}
}

void update_score( void ) {
	if( score > MAX_SCORE ) score = MAX_SCORE;
	text_write_number( 1, 1, score, ALIGN_LEFT, true );
}

void update_charge( void ) {
	int i;
	char offset = 0;

	for( i=0 ; i<10 ; i++ ) {
		if( bullet_charge > (BULLET_CHARGE_MAX/10)*i )
			offset = 52;
		else
			offset = 49;
		
		if( i == 0 )
			offset--;
		else if( i == 9 )
			offset++;		
		
		vram[(VRAM_TILES_H*VRAM_TILES_V)+i+9] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + offset;
	}
}

void update_lives() {
	if( lives > MAX_LIVES ) lives = MAX_LIVES;
	vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+26] = lives + 32 + OVERLAY_OFFSET(level) + RAM_TILES_COUNT;
}

void init_overlay() {
	int i;

	// Clear overlay
	for( i=0 ; i<VRAM_TILES_H ; i++ ) {
		vram[(VRAM_TILES_H*VRAM_TILES_V)+i] = RAM_TILES_COUNT;
		vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+i] = RAM_TILES_COUNT;
	}

	// "Score" text
	vram[(VRAM_TILES_H*VRAM_TILES_V)+1] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 54;
	vram[(VRAM_TILES_H*VRAM_TILES_V)+2] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 55;
	vram[(VRAM_TILES_H*VRAM_TILES_V)+3] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 56;

	// "Charge" text
	vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+12] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 57;
	vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+13] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 58;
	vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+14] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 59;
	vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+15] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 60;
	
	// Lives counter
	vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+24] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 61;
	vram[(VRAM_TILES_H*(VRAM_TILES_V+1))+25] = OVERLAY_OFFSET(level) + RAM_TILES_COUNT + 62;

	update_score();
	update_charge();
	update_lives();
}

unsigned int col_check( int sprite, int *tile_x, int *tile_y ) {
		unsigned char smap = pgm_read_byte( &sprite_col_map[sprites[sprite].tileIndex] );

		if( smap==0 ) {
			// If all empty, no chance of collision.
			return 0;
		}
		else {
			int x = ((Screen.scrollX + sprites[sprite].x) / 8) % VRAM_TILES_H;
			int offset_x = (Screen.scrollX + sprites[sprite].x) % 8;
			int y = sprites[sprite].y / 8;
			int offset_y = sprites[sprite].y % 8;

			// This is the tile the top-left corner of the sprite occupies.
			unsigned char *tile = &vram[(y*VRAM_TILES_H) + x];
			
			// Build up a bitmap representing a 2x2 grid extending from the tile.
			// +-----+-----+
			// |15 14|11 10|
			// |13 12| 9  8|
			// +-----+-----+
			// | 7  6| 3  2|
			// | 5  4| 1  0|
			// +-----+-----+
			unsigned int t = pgm_read_byte( &bg_col_map[(*tile)-RAM_TILES_COUNT]) << 12;
			if( x == VRAM_TILES_H-1 ) {
				t |= pgm_read_byte( &bg_col_map[(*(tile-VRAM_TILES_H-1))-RAM_TILES_COUNT] ) << 8;
			}
			else {
				t |= pgm_read_byte( &bg_col_map[(*(tile+1))-RAM_TILES_COUNT] ) << 8;
			}

			// Fill bottom row?
			if( y < LEVEL_TILES_Y-1 ) {
				t |= pgm_read_byte( &bg_col_map[(*(tile+VRAM_TILES_H))-RAM_TILES_COUNT] ) << 4;
				if( x == VRAM_TILES_H-1 ) {
					t |= pgm_read_byte( &bg_col_map[(*(tile+1))-RAM_TILES_COUNT] );
				}
				else {
					t |= pgm_read_byte( &bg_col_map[(*(tile+VRAM_TILES_H+1))-RAM_TILES_COUNT] );
				}
			}

			// No chance of collision if all tiles are empty.
			if( t == 0 ) {
				return 0;
			}

			// Do the same for the sprite, taking the offset into the 2x2
			// tile grid into account.
			unsigned int s = smap << 12;
			if( offset_x != 0 ) {
				// OR to the right
				s |= ((s & 0xa000) >> 1)
  				  |  ((s & 0x5000) >> 3);
				if( offset_x > 3 ) {
					// Shift right
					s = ((s & 0xa000) >> 1)
					  | ((s & 0x5000) >> 3)
					  | ((s & 0x0a00) >> 1);
				}
			}

			if( offset_y > 0 ) {
				// OR downwards
				s |= ((s & 0xcc00) >> 2)
				  |  ((s & 0x3300) >> 6);
				if( offset_y > 3 ) {
					// Shift downwards
					s = ((s & 0xcc00) >> 2)
					  | ((s & 0x3300) >> 6)
					  | ((s & 0x00cc) >> 2);
				}
			}
	
			// Now just AND the bitmasks - a collision will produce > 0;
			if( s & t ) {
				*tile_x = x;
				*tile_y = y;
				return s & t;
			}
		}
		return 0;
}

void check_enemy_hit( int x, int y, bullet_status_t b ) {
	int i;
	char hit = 0;
	
	for( i=0 ; i<MAX_ENEMIES ; i++ ) {
		switch( enemies[i].id ) {
			case ENEMY_NONE:
				break;
			case ENEMY_MINE:
			case ENEMY_MORTAR_LAUNCHER:
				if((x == enemies[i].x || x == enemies[i].x+1)
				&& (y == enemies[i].y || y == enemies[i].y+1) ) {
					hit = 1;
				}
				break;
			case ENEMY_SPINNER:
				if((x >= enemies[i].x && x <= enemies[i].x+2)
				&& (y == enemies[i].y || y == enemies[i].y+1) ) {
					hit = 1;
				}
				break;

			case ENEMY_EYEBALL:
				if((x == enemies[i].x || x == enemies[i].x+1)
				&& (y == enemies[i].y || y == enemies[i].y+1)
				&& (enemies[i].anim_step >= 420 && enemies[i].anim_step <= 780) ) {
					hit = 1;
				}
				break;
			default:
				break;
		}
		if( hit ) {
			if( enemies[i].hp != HP_INFINITE ) {
				switch( b ) {
					case BULLET_SMALL:
						enemies[i].hp--;
						break;
					case BULLET_MEDIUM:
						enemies[i].hp -= 4;
						break;
					case BULLET_LARGE:
						if( ! enemies[i].whooshed ) {
							enemies[i].hp -= 8;
							enemies[i].whooshed = 1;
						}
						break;
					default:
						break;
				}
				if( enemies[i].hp <= 0 ) {
					TriggerFx( SFX_EXP_S, 0xff, true );
					score += pgm_read_word( &enemy_score[enemies[i].id] );
					switch( enemies[i].id ) {
						case ENEMY_EYEBALL:
							fill_tiles( enemies[i].x-VRAM_TILES_H+5, enemies[i].y+1, VRAM_TILES_H-5, 2, 0 );
							fill_tiles( enemies[i].x, enemies[i].y-1, 1, 4, 0 );
							draw_enemy( enemies[i].x+1, enemies[i].y-1, dead_eyeball_map );
							// Fall through...
						case ENEMY_MINE:
						case ENEMY_MORTAR_LAUNCHER:
							enemies[i].id = ENEMY_EXP_2X2;
							enemies[i].anim_step = 0;
							break;
						case ENEMY_SPINNER:
							enemies[i].id = ENEMY_EXP_3X2;
							enemies[i].anim_step = 0;
							break;
						default:
							break;
					}
				}
				return;
			}
		}
	}
}

void check_colmap_hit( unsigned int col_map, int col_x, int col_y, bullet_status_t b ) {
	if( col_map & 0xf000 ) {
		check_enemy_hit( col_x, col_y, b );
	}
	if( col_map & 0x0f00 ) {
		check_enemy_hit( col_x+1, col_y, b );
	}
	if( col_map & 0x00f0 ) {
		check_enemy_hit( col_x, col_y+1, b );
	}
	if( col_map & 0x000f ) {
		check_enemy_hit( col_x+1, col_y+1, b );
	}
}

void start_level( int level ){
	int i;
	bool done = false;
	int current_bullet = -1;
	long old_score = 0;
	unsigned int col_map;
	int col_x, col_y;

	FadeOut(FADE_SPEED,true);

	score = 0;
	bullet_charge = 0;
	frame = 0;

	clear_enemies();
	enemy_pos = level1_enemies;
	clear_sprites();
	SetTileTable(tiles1);
	SetSpriteVisibility(true);
	SetScrolling(0,0);
	Screen.overlayHeight=OVERLAY_LINES;
	level_load( (unsigned char*)level1_map );

	sprites[0].tileIndex = 1;
	sprites[1].tileIndex = 0x09;
	sprites[2].tileIndex = 0x0a;
	sprites[3].tileIndex = 0x0b;

	ship.x = 16;
	ship.y = SHIP_MAX_Y/2;
	ship.speed = 1;
	ship.status = STATUS_OK;
	
	init_overlay();

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
				if( current_bullet == -1 ) {
					current_bullet = new_bullet();
				}
				if( current_bullet != -1 ) {
					bullet_charge++;
					if( bullet_charge > BULLET_CHARGE_MAX ) {
						bullet_charge = BULLET_CHARGE_MAX;
						if( frame % 16 == 0 ) {
							TriggerFx( SFX_CHARGED, 0xff, true );
						}
					}
					update_charge();
				}
			}
			else {
				if( current_bullet != -1 ) {
					if( bullet_charge == BULLET_CHARGE_MAX ) {
						set_bullet( current_bullet, BULLET_LARGE );
					}
					else if( bullet_charge >= BULLET_CHARGE_MAX/2 ) {
						set_bullet( current_bullet, BULLET_MEDIUM );
					}
					else {
						set_bullet( current_bullet, BULLET_SMALL );
					}
					bullet_charge = 0;
					update_charge();
					current_bullet = -1;
				}
			}
			sprites[0].x = ship.x;      sprites[0].y = ship.y;
			sprites[1].x = ship.x;      sprites[1].y = ship.y + 8;
			sprites[2].x = ship.x + 8;  sprites[2].y = ship.y + 8;
			sprites[3].x = ship.x + 16; sprites[3].y = ship.y + 8;
		}
		else if( ship.status == STATUS_EXPLODING ) {
			if( ship.anim_step == 16 ) {
				TriggerFx( SFX_EXP_L, 0xff, true );
				MapSprite( 0, ship_explosion_map[0] );
			}

			if( ship.anim_step == 8 ) {
				TriggerFx( SFX_EXP_L, 0xff, true );
				MapSprite( 0, ship_explosion_map[1] );
			}
			
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
				col_map = col_check( i, &col_x, &col_y );
				if( col_map ) {
					if( i<SPRITE_BULLET1 ) {
						// Ship has crashed
						if( ship.status != STATUS_EXPLODING ) {
							ship.status = STATUS_EXPLODING;
							ship.anim_step = 16;
							sprites[3].x -= 8;
							sprites[3].y -= 8;
						}
					}
					else if( i < SPRITE_BULLET1+MAX_BULLETS ) {
						// Bullet hit something...
						check_colmap_hit( col_map, col_x, col_y, bullet[i-SPRITE_BULLET1].status );
						set_bullet( i-SPRITE_BULLET1, BULLET_FREE );
					}
					else if( i < SPRITE_WHOOSH+WHOOSH_SPRITES ) {
						// Same as bullet, but don't erase the whoosh.
						check_colmap_hit( col_map, col_x, col_y, BULLET_LARGE );
					}
				}
			}
		}

		if( score != old_score ) {
			update_score();
			old_score = score;
		}

		update_enemies();

		frame++;
	}

	if( Screen.scrollY != 0 ) SetScrolling( Screen.scrollX, 0 );
	WaitVsync(60);
	FadeOut(FADE_SPEED,true);
	SetSpriteVisibility(false);
	ClearVram();
}

void draw_starfield( void ) {
	int i;

	srandom(43);
	for( i=0 ; i<30 ; i++ ) {
		SetTile(
			random()%SCREEN_TILES_H,
			random()%SCREEN_TILES_V,
			(random()%3) + OVERLAY_OFFSET(level) + 45);
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
	text_write((SCREEN_TILES_H-20)/2,23,"c2011 STEVE MADDISON",false);	
	FadeIn(FADE_SPEED,false);

	for( i=0 ; i<TITLE_SECONDS ; i++ ) {
		text_write((SCREEN_TILES_H-10)/2,19,"PUSH START",false);
		if( wait_start(FPS/2) ) return 1;

		text_write((SCREEN_TILES_H-10)/2,19,"          ",false);
		if( wait_start(FPS/2) ) return 1;
	}
	return 0;
}

int show_hi_scores() {
#define HI_SCORE_TOP 10
	int i;

	FadeOut(FADE_SPEED,true);
	ClearVram();
	draw_starfield();

	for( i=0 ; i<SCREEN_TILES_H ; i++ ) {
		SetTile(i,3,OVERLAY_OFFSET(level)+52);
		SetTile(i,22,OVERLAY_OFFSET(level)+52);
	}
	text_write((SCREEN_TILES_H-10)/2,HI_SCORE_TOP-4,"HI  SCORES",false);

	for( i=0 ; i<HIGH_SCORES ; i++ ) {
		text_write_number(6,HI_SCORE_TOP+i,i+1,ALIGN_RIGHT,false);
		text_write(7,HI_SCORE_TOP+i,".",false);
		text_write(9,HI_SCORE_TOP+i,hi_name[i],false);
		text_write_number(21,HI_SCORE_TOP+i,hi_score[i],ALIGN_RIGHT,false);
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
	InitMusicPlayer(patches);
	while(1) {
		Screen.overlayHeight=0;
		SetScrolling(0,0);
		SetTileTable(tiles1);
		if( show_title() || show_attract() || show_hi_scores() ) {
			// Start was pressed...
			show_intro();
			start_level(1);
		}
	}
}

