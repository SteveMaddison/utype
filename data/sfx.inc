// Short explosion
const char patch_exp_s[] PROGMEM = {
	0,PC_ENV_VOL,0xa0,
	0,PC_NOISE_PARAMS,0x07,
	0,PC_ENV_SPEED,-4,
	0,PATCH_END
};

// Long explosion
const char patch_exp_l[] PROGMEM = {
	0,PC_ENV_VOL,0xf0,
	0,PC_ENV_SPEED,-2,
	0,PC_NOISE_PARAMS,0x7,
	10,PC_ENV_VOL,0xa0,
	10,PC_ENV_VOL,0x80,
	10,PC_ENV_VOL,0x60,
	0,PATCH_END
};

// Fire blaster
const char patch_fire[] PROGMEM = {
	0,PC_WAVE,1,
	0,PC_ENV_VOL,0xa0,
	0,PC_ENV_SPEED,-8,
	0,PC_PITCH,90,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,8,
	1,PC_NOTE_DOWN,8,
	2,PC_NOTE_CUT,0,
	0,PATCH_END
};

// Beam charge
const char patch_charging[] PROGMEM = {
	0,PC_WAVE,2,
	0,PC_ENV_VOL,0x40,
	0,PC_ENV_SPEED,4,
	0,PC_PITCH,25,
	3,PC_NOTE_UP,4,
	3,PC_NOTE_UP,4,
	3,PC_NOTE_UP,4,
	3,PC_NOTE_UP,4,
	3,PC_NOTE_UP,4,
	3,PC_NOTE_UP,3,
	3,PC_NOTE_UP,3,
	3,PC_NOTE_UP,3,
	3,PC_NOTE_UP,3,
	3,PC_NOTE_UP,2,
	3,PC_NOTE_UP,2,
	3,PC_NOTE_UP,2,
	3,PC_NOTE_UP,1,
	3,PC_NOTE_UP,1,
	3,PC_NOTE_UP,1,
	3,PC_NOTE_UP,1,
	3,PC_NOTE_UP,1,
	3,PC_NOTE_CUT,0,
	0,PATCH_END
};

const char patch_charged[] PROGMEM = {
	0,PC_WAVE,2,
	0,PC_TREMOLO_LEVEL,0x80,
	0,PC_TREMOLO_RATE,60,
	0,PC_PITCH,60,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_UP,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_DOWN,4,
	1,PC_NOTE_CUT,0,
	0,PATCH_END
};

// Whoosh!
const char patch_whoosh[] PROGMEM = { 
	0,PC_ENV_VOL,0x80,
	0,PC_ENV_SPEED,4,
	0,PC_NOISE_PARAMS,0x05,
	10,PC_ENV_SPEED,-2,
	20,PATCH_END
};

typedef enum {
	SFX_EXP_S = 0,
	SFX_EXP_L,
	SFX_FIRE,
	SFX_CHARGING,
	SFX_CHARGED,
	SFX_WHOOSH,
} sfx_t;

const struct PatchStruct patches[] PROGMEM = {
	{1,NULL,patch_exp_s,0,0},
	{1,NULL,patch_exp_l,0,0},
	{0,NULL,patch_fire,0,0},
	{0,NULL,patch_charging,0,0},
	{0,NULL,patch_charged,0,0},
	{1,NULL,patch_whoosh,0,0},
};

