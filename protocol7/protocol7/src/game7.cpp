/*
* game7.cpp - Enhanced game structure:
*				- Dynamic level generation
*				- Multiple levels
*/
#include "stdafx.h"
#include "base.h"
#include "sys.h"
#include "core.h"

//=============================================================================
void Log(LPTSTR lpFormat, ...)
{
	TCHAR szBuf[1024];
	va_list marker;

	va_start(marker, lpFormat);
	vsprintf(szBuf, lpFormat, marker);
	OutputDebugString(szBuf);
	va_end(marker);
}
#define LOG(ALL_ARGS) Log ALL_ARGS
//#define LOG(ALL_ARGS) do{} while(0)

//=============================================================================
// Utility functions
template <typename T>
inline T SafeSub(const T& base, const T& chunk) { return (base - chunk > 0 ? base - chunk : 0); }
template <typename T>
inline T SafeAdd(const T& base, const T& chunk, const T& max)
{
	return (base + chunk <= max ? base + chunk : max);
}
template <typename T>
inline T Max(const T& a, const T& b) { return (a > b ? a : b); }
template <typename T>
inline T Min(const T& a, const T& b) { return (a < b ? a : b); }

//=============================================================================
// Game Parameter Constants

// Sprites
static const float SPRITE_SCALE = 8.f;
static const float SHADOW_OFFSET = 80.f;
static const float SHADOW_SCALE = 0.9f;

// Ship boundaries
static const size_t MAINSHIP_ENTITY = 0;
static const float MAINSHIP_RADIUS = 100.f;
static const float ROCK_RADIUS = 100.f;
static const float JUICE_RADIUS = 100.f;
static const float SHIP_W = 250;
static const float SHIP_H = 270;

// Ship movement
static const float SHIP_MIN_SPEED = 15.f;
static const float SHIP_MAX_SPEED = 25.f;
static const float SHIP_CRUISE_SPEED = 25.f;
static const float SHIP_START_SPEED = 1.f;
static const float SHIP_INC_SPEED = .1f;
static const float SHIP_TILT_INC = .2f;
static const float SHIP_TILT_FRICTION = .1f;
static const float SHIP_MAX_TILT = 1.5f;
static const float SHIP_HVEL_FRICTION = .1f;
static const float CRASH_VEL = 20.f;

// Ship resources
static const float MAX_ENERGY = 100.f;
static const float MAX_FUEL = 100.f;
static const float TILT_FUEL_COST = .03f;
static const float FRAME_FUEL_COST = .01f;
static const float ROCK_CRASH_ENERGY_LOSS = 30.f;
static const float MINE_CRASH_ENERGY_LOSS = 80.f;
static const float MIN_FUEL_FOR_HEAL = MAX_FUEL / 2.f;
static const float FUEL_HEAL_PER_FRAME = 0.2f;
static const float ENERGY_HEAL_PER_FRAME = 0.1f;
static const float JUICE_FUEL = 30.f;

// Rockets
static const float ROCKET_SPEED = 15.f;
static const float MIN_TIME_BETWEEN_ROCKETS = 1.f;
static const float ROCKET_RADIUS = 50.f;

// Resource UI
static const float ENERGY_BAR_W = 60.f;
static const float ENERGY_BAR_H = 1500.f;
static const float FUEL_BAR_W = 60.f;
static const float FUEL_BAR_H = 1500.f;
static const float CHUNK_W = 40.f;
static const float CHUNK_H = 40.f;
static const size_t MAX_CHUNKS = 30;

// Obstacles
static const float START_ROCK_CHANCE_PER_PIXEL = 1.f / 1000.f;
static const float EXTRA_ROCK_CHANCE_PER_PIXEL = 0.f;	//1.f/2500000.f;
static const float JUICE_CHANCE_PER_PIXEL = 1.f / 10000.f;
static const float GEN_IN_ADVANCE = 400.f;

// Game state
static const float RACE_END = 100000.f;
static const float FPS = 60.f;
static const float FRAMETIME = (1.f / FPS);

static const float STARTING_TIME = 1.f;
static const float DYING_TIME = 2.f;
static const float VICTORY_TIME = 8.f;

static const float FIRST_CHALLENGE = 3000.f;

static const float SND_DEFAULT_VOL = .7f;

// Level generation
//static const float PATH_TWIST_RATIO = 0.5f; // This means about 30 degrees maximum
//static const float PATH_WIDTH = (2.f * MAINSHIP_RADIUS);
float g_next_challenge_area = FIRST_CHALLENGE;
vec2 g_last_conditioned = vmake(0.f, 0.f);

float volatile g_current_race_pos = 0.f;
float volatile g_camera_offset = 0.f;
float g_rock_chance = START_ROCK_CHANCE_PER_PIXEL;
float g_time_from_last_rocket = 0.f;

//=============================================================================
// Game state
enum GameState { GS_START, GS_PLAYING, GS_DYING, GS_VICTORY };
GameState g_gs = GS_START;
float volatile g_gs_timer = 0.f;

//=============================================================================
// Textures management
enum TexId
{
	T_FONT,
	T_PARTICLE,
	T_SHIP_LL,
	T_SHIP_L,
	T_SHIP_C,
	T_SHIP_R,
	T_SHIP_RR,
	T_BKG0,
	T_ROCK0,
	T_ROCK1,
	T_ROCK2,
	T_ROCK3,
	T_ROCK4,
	T_PEARL,
	T_ENERGY,
	T_FUEL,
	T_STAR,
	T_JUICE,
	T_ROCKET,
	T_MINE,
	T_DRONE0,
	T_DRONE1,
	T_DRONE2,

	// Tiles
	T_TILES_G_ON_S,
	T_SSSS = T_TILES_G_ON_S, T_SSSG, T_SSGS, T_SSGG,
	T_SGSS, T_SGSG, T_SGGS, T_SGGG,
	T_GSSS, T_GSSG, T_GSGS, T_GSGG,
	T_GGSS, T_GGSG, T_GGGS, T_GGGG,

	T_TILES_D_ON_S,
	T_SSSS_2 = T_TILES_D_ON_S, T_SSSD, T_SSDS, T_SSDD,
	T_SDSS, T_SDSD, T_SDDS, T_SDDD,
	T_DSSS, T_DSSD, T_DSDS, T_DSDD,
	T_DDSS, T_DDSD, T_DDDS, T_DDDD,

	T_TILES_E_ON_D,
	T_DDDD_2 = T_TILES_E_ON_D, T_DDDE, T_DDED, T_DDEE,
	T_DEDD, T_DEDE, T_DEED, T_DEEE,
	T_EDDD, T_EDDE, T_EDED, T_EDEE,
	T_EEDD, T_EEDE, T_EEED, T_EEEE,

	T_TILES_P_ON_S,
	T_SSSS_3 = T_TILES_P_ON_S, T_SSSP, T_SSPS, T_SSPP,
	T_SPSS, T_SPSP, T_SPPS, T_SPPP,
	T_PSSS, T_PSSP, T_PSPS, T_PSPP,
	T_PPSS, T_PPSP, T_PPPS, T_PPPP,

	T_TILES_I_ON_W,
	T_WWWW = T_TILES_I_ON_W, T_WWWI, T_WWIW, T_WWII,
	T_WIWW, T_WIWI, T_WIIW, T_WIII,
	T_IWWW, T_IWWI, T_IWIW, T_IWII,
	T_IIWW, T_IIWI, T_IIIW, T_IIII,

	T_TILES_R_ON_L,
	T_LLLL = T_TILES_R_ON_L, T_LLLR, T_LLRL, T_LLRR,
	T_LRLL, T_LRLR, T_LRRL, T_LRRR,
	T_RLLL, T_RLLR, T_RLRL, T_RLRR,
	T_RRLL, T_RRLR, T_RRRL, T_RRRR,

	T_TILES_Y_ON_X,
	T_XXXX = T_TILES_Y_ON_X, T_XXXY, T_XXYX, T_XXYY,
	T_XYXX, T_XYXY, T_XYYX, T_XYYY,
	T_YXXX, T_YXXY, T_YXYX, T_YXYY,
	T_YYXX, T_YYXY, T_YYYX, T_YYYY,

	T_TILES_V_ON_K,
	T_KKKK_2 = T_TILES_V_ON_K, T_KKKV, T_KKVK, T_KKVV,
	T_KVKK, T_KVKV, T_KVVK, T_KVVV,
	T_VKKK, T_VKKV, T_VKVK, T_VKVV,
	T_VVKK, T_VVKV, T_VVVK, T_VVVV,

	T_TILES_B_ON_K,
	T_KKKK = T_TILES_B_ON_K, T_KKKB, T_KKBK, T_KKBB,
	T_KBKK, T_KBKB, T_KBBK, T_KBBB,
	T_BKKK, T_BKKB, T_BKBK, T_BKBB,
	T_BBKK, T_BBKB, T_BBBK, T_BBBB
};

struct TextureManager
{
	char   name[100];
	bool   wrap;
	GLuint tex;
};

TextureManager textures[] =
{
	{"data/Kromasky.bmp" , false, 0},
	{"data/Particle.bmp" , false, 0},
	{"data/ShipLL.bmp"   , false, 0},
	{"data/ShipL.bmp"    , false, 0},
	{"data/ShipC.bmp"    , false, 0},
	{"data/ShipR.bmp"    , false, 0},
	{"data/ShipRR.bmp"   , false, 0},
	{"data/bkg0.bmp"     , false, 0},
	{"data/Rock0.bmp"    , false, 0},
	{"data/Rock1.bmp"    , false, 0},
	{"data/Rock2.bmp"    , false, 0},
	{"data/Rock3.bmp"    , false, 0},
	{"data/Rock4.bmp"    , false, 0},
	{"data/Pearl.bmp"    , false, 0},
	{"data/Energy.bmp"   , false, 0},
	{"data/Fuel.bmp"     , false, 0},
	{"data/Star.bmp"     , false, 0},
	{"data/Juice.bmp"    , false, 0},
	{"data/Rocket.bmp"   , false, 0},
	{"data/Mine.bmp"     , false, 0},
	{"data/Drone0.bmp"   , false, 0},
	{"data/Drone1.bmp"   , false, 0},
	{"data/Drone2.bmp"   , false, 0},

	// Terrain tiles
	// Sand-grass
	{ "data/tiles/sg/ssss.bmp"  , false, 0 },
	{"data/tiles/sg/sssg.bmp"  , false, 0},
	{"data/tiles/sg/ssgs.bmp"  , false, 0},
	{"data/tiles/sg/ssgg.bmp"  , false, 0},
	{"data/tiles/sg/sgss.bmp"  , false, 0},
	{"data/tiles/sg/sgsg.bmp"  , false, 0},
	{"data/tiles/sg/sggs.bmp"  , false, 0},
	{"data/tiles/sg/sggg.bmp"  , false, 0},
	{"data/tiles/sg/gsss.bmp"  , false, 0},
	{"data/tiles/sg/gssg.bmp"  , false, 0},
	{"data/tiles/sg/gsgs.bmp"  , false, 0},
	{"data/tiles/sg/gsgg.bmp"  , false, 0},
	{"data/tiles/sg/ggss.bmp"  , false, 0},
	{"data/tiles/sg/ggsg.bmp"  , false, 0},
	{"data/tiles/sg/gggs.bmp"  , false, 0},
	{"data/tiles/sg/gggg1.bmp" , false, 0},

	// Sand-dark
	{"data/tiles/sd/ssss.bmp"  , false, 0},
	{"data/tiles/sd/sssd.bmp"  , false, 0},
	{"data/tiles/sd/ssds.bmp"  , false, 0},
	{"data/tiles/sd/ssdd.bmp"  , false, 0},
	{"data/tiles/sd/sdss.bmp"  , false, 0},
	{"data/tiles/sd/sdsd.bmp"  , false, 0},
	{"data/tiles/sd/sdds.bmp"  , false, 0},
	{"data/tiles/sd/sddd.bmp"  , false, 0},
	{"data/tiles/sd/dsss.bmp"  , false, 0},
	{"data/tiles/sd/dssd.bmp"  , false, 0},
	{"data/tiles/sd/dsds.bmp"  , false, 0},
	{"data/tiles/sd/dsdd.bmp"  , false, 0},
	{"data/tiles/sd/ddss.bmp"  , false, 0},
	{"data/tiles/sd/ddsd.bmp"  , false, 0},
	{"data/tiles/sd/ddds.bmp"  , false, 0},
	{"data/tiles/sd/dddd.bmp"  , false, 0},

	// Elevated-dark
	{"data/tiles/de/dddd.bmp"  , false, 0},
	{"data/tiles/de/ddde.bmp"  , false, 0},
	{"data/tiles/de/dded.bmp"  , false, 0},
	{"data/tiles/de/ddee.bmp"  , false, 0},
	{"data/tiles/de/dedd.bmp"  , false, 0},
	{"data/tiles/de/dede.bmp"  , false, 0},
	{"data/tiles/de/deed.bmp"  , false, 0},
	{"data/tiles/de/deee.bmp"  , false, 0},
	{"data/tiles/de/eddd.bmp"  , false, 0},
	{"data/tiles/de/edde.bmp"  , false, 0},
	{"data/tiles/de/eded.bmp"  , false, 0},
	{"data/tiles/de/edee.bmp"  , false, 0},
	{"data/tiles/de/eedd.bmp"  , false, 0},
	{"data/tiles/de/eede.bmp"  , false, 0},
	{"data/tiles/de/eeed.bmp"  , false, 0},
	{"data/tiles/de/eeee.bmp"  , false, 0},

	// Sand-palm
	{"data/tiles/sp/ssss.bmp"  , false, 0},
	{"data/tiles/sp/sssp.bmp"  , false, 0},
	{"data/tiles/sp/ssps.bmp"  , false, 0},
	{"data/tiles/sp/sspp.bmp"  , false, 0},
	{"data/tiles/sp/spss.bmp"  , false, 0},
	{"data/tiles/sp/spsp.bmp"  , false, 0},
	{"data/tiles/sp/spps.bmp"  , false, 0},
	{"data/tiles/sp/sppp.bmp"  , false, 0},
	{"data/tiles/sp/psss.bmp"  , false, 0},
	{"data/tiles/sp/pssp.bmp"  , false, 0},
	{"data/tiles/sp/psps.bmp"  , false, 0},
	{"data/tiles/sp/pspp.bmp"  , false, 0},
	{"data/tiles/sp/ppss.bmp"  , false, 0},
	{"data/tiles/sp/ppsp.bmp"  , false, 0},
	{"data/tiles/sp/ppps.bmp"  , false, 0},
	{"data/tiles/sp/pppp.bmp"  , false, 0},

	// Water-island
	{"data/tiles/wi/wwww.bmp"  , false, 0},
	{"data/tiles/wi/wwwi.bmp"  , false, 0},
	{"data/tiles/wi/wwiw.bmp"  , false, 0},
	{"data/tiles/wi/wwii.bmp"  , false, 0},
	{"data/tiles/wi/wiww.bmp"  , false, 0},
	{"data/tiles/wi/wiwi.bmp"  , false, 0},
	{"data/tiles/wi/wiiw.bmp"  , false, 0},
	{"data/tiles/wi/wiii.bmp"  , false, 0},
	{"data/tiles/wi/iwww.bmp"  , false, 0},
	{"data/tiles/wi/iwwi.bmp"  , false, 0},
	{"data/tiles/wi/iwiw.bmp"  , false, 0},
	{"data/tiles/wi/iwii.bmp"  , false, 0},
	{"data/tiles/wi/iiww.bmp"  , false, 0},
	{"data/tiles/wi/iiwi.bmp"  , false, 0},
	{"data/tiles/wi/iiiw.bmp"  , false, 0},
	{"data/tiles/wi/iiii.bmp"  , false, 0},

	// L-R
	{"data/tiles/lr/llll.bmp"  , false, 0},
	{"data/tiles/lr/lllr.bmp"  , false, 0},
	{"data/tiles/lr/llrl.bmp"  , false, 0},
	{"data/tiles/lr/llrr.bmp"  , false, 0},
	{"data/tiles/lr/lrll.bmp"  , false, 0},
	{"data/tiles/lr/lrlr.bmp"  , false, 0},
	{"data/tiles/lr/lrrl.bmp"  , false, 0},
	{"data/tiles/lr/lrrr.bmp"  , false, 0},
	{"data/tiles/lr/rlll.bmp"  , false, 0},
	{"data/tiles/lr/rllr.bmp"  , false, 0},
	{"data/tiles/lr/rlrl.bmp"  , false, 0},
	{"data/tiles/lr/rlrr.bmp"  , false, 0},
	{"data/tiles/lr/rrll.bmp"  , false, 0},
	{"data/tiles/lr/rrlr.bmp"  , false, 0},
	{"data/tiles/lr/rrrl.bmp"  , false, 0},
	{"data/tiles/lr/rrrr.bmp"  , false, 0},

	// X-Y
	{"data/tiles/xy/xxxx.bmp"  , false, 0},
	{"data/tiles/xy/xxxy.bmp"  , false, 0},
	{"data/tiles/xy/xxyx.bmp"  , false, 0},
	{"data/tiles/xy/xxyy.bmp"  , false, 0},
	{"data/tiles/xy/xyxx.bmp"  , false, 0},
	{"data/tiles/xy/xyxy.bmp"  , false, 0},
	{"data/tiles/xy/xyyx.bmp"  , false, 0},
	{"data/tiles/xy/xyyy.bmp"  , false, 0},
	{"data/tiles/xy/yxxx.bmp"  , false, 0},
	{"data/tiles/xy/yxxy.bmp"  , false, 0},
	{"data/tiles/xy/yxyx.bmp"  , false, 0},
	{"data/tiles/xy/yxyy.bmp"  , false, 0},
	{"data/tiles/xy/yyxx.bmp"  , false, 0},
	{"data/tiles/xy/yyxy.bmp"  , false, 0},
	{"data/tiles/xy/yyyx.bmp"  , false, 0},
	{"data/tiles/xy/yyyy.bmp"  , false, 0},

	// K-V
	{"data/tiles/kv/kkkk.bmp"  , false, 0},
	{"data/tiles/kv/kkkv.bmp"  , false, 0},
	{"data/tiles/kv/kkvk.bmp"  , false, 0},
	{"data/tiles/kv/kkvv.bmp"  , false, 0},
	{"data/tiles/kv/kvkk.bmp"  , false, 0},
	{"data/tiles/kv/kvkv.bmp"  , false, 0},
	{"data/tiles/kv/kvvk.bmp"  , false, 0},
	{"data/tiles/kv/kvvv.bmp"  , false, 0},
	{"data/tiles/kv/vkkk.bmp"  , false, 0},
	{"data/tiles/kv/vkkv.bmp"  , false, 0},
	{"data/tiles/kv/vkvk.bmp"  , false, 0},
	{"data/tiles/kv/vkvv.bmp"  , false, 0},
	{"data/tiles/kv/vvkk.bmp"  , false, 0},
	{"data/tiles/kv/vvkv.bmp"  , false, 0},
	{"data/tiles/kv/vvvk.bmp"  , false, 0},
	{"data/tiles/kv/vvvv1.bmp" , false, 0},

	// K-B
	{"data/tiles/kb/kkkk.bmp"  , false, 0},
	{"data/tiles/kb/kkkb.bmp"  , false, 0},
	{"data/tiles/kb/kkbk.bmp"  , false, 0},
	{"data/tiles/kb/kkbb.bmp"  , false, 0},
	{"data/tiles/kb/kbkk.bmp"  , false, 0},
	{"data/tiles/kb/kbkb.bmp"  , false, 0},
	{"data/tiles/kb/kbbk.bmp"  , false, 0},
	{"data/tiles/kb/kbbb.bmp"  , false, 0},
	{"data/tiles/kb/bkkk.bmp"  , false, 0},
	{"data/tiles/kb/bkkb.bmp"  , false, 0},
	{"data/tiles/kb/bkbk.bmp"  , false, 0},
	{"data/tiles/kb/bkbb.bmp"  , false, 0},
	{"data/tiles/kb/bbkk.bmp"  , false, 0},
	{"data/tiles/kb/bbkb.bmp"  , false, 0},
	{"data/tiles/kb/bbbk.bmp"  , false, 0},
	{"data/tiles/kb/bbbb.bmp"  , false, 0},
};

void LoadTextures()
{
	for (size_t i = 0; i < ArraySize(textures); i++)
		textures[i].tex = CORE_LoadBmp(textures[i].name, true);
}

void UnloadTextures()
{
	for (size_t i = 0; i < ArraySize(textures); i++)
		CORE_UnloadBmp(textures[i].tex);
}

inline GLuint Tex(TexId id) { return textures[id].tex; }

//=============================================================================
// Sound engine
enum SoundId
{
	SND_THUMP,
	SND_EXPLOSION,
	SND_ENGINE,
	SND_SUCCESS
};

struct Sound
{
	char name[100];
	int  buf_id;
};

Sound sounds[] =
{
	{"data/thump.wav", 0},
	{"data/explosion.wav", 0},
	{"data/ffff.wav", 0},
	{"data/success.wav", 0}
};

void LoadSounds()
{
	for (size_t i = 0; i < ArraySize(sounds); i++)
		sounds[i].buf_id = CORE_LoadWav(sounds[i].name);
}

void UnloadSounds()
{
	for (size_t i = 0; i < ArraySize(sounds); i++)
		CORE_UnloadWav(sounds[i].buf_id);
}

void PlaySound(SoundId id, float vol = SND_DEFAULT_VOL, float freq = 1.f)
{
	CORE_PlaySound(sounds[id].buf_id, vol, freq);
}

void PlayLoopSound(unsigned loop_channel, SoundId id, float vol, float pitch)
{
	CORE_PlayLoopSound(loop_channel, sounds[id].buf_id, vol, pitch);
}

void SetLoopSoundParam(unsigned loop_channel, float vol, float pitch)
{
	CORE_SetLoopSoundParam(loop_channel, vol, pitch);
}

//=============================================================================
// Particle systems manager
enum PSType
{
	PST_NULL, PST_WATER, PST_FIRE, PST_SMOKE, PST_DUST, PST_GOLD
};

struct PSDef
{
	TexId  texture;
	bool   additive;
	size_t new_parts_per_frame;
	int    death;
	vec2   force;
	float  start_pos_random;
	vec2   start_speed_fixed;
	float  start_speed_random;
	float  start_radius_min, startradius_max;
	rgba   start_color_fixed;
	rgba   start_color_random;
};

PSDef psdefs[] =
{
	//            TEX          ADD     N  DTH  FORCE               rndPos SPEED           rndSPD  Rmin  Rmax
	//	clr                      clr-rand
	/* null  */{},
	/* water */{T_PARTICLE,  true ,  4, 150, vmake(0.f, -0.025f),  4.f,  vmake(0.f, 0.45f), .1f,  8.f, 10.f,
	RGBA(64,  64, 255, 128), RGBA(0, 0, 0, 0)},

	/* fire  */{T_PARTICLE,  true ,  8,  60, vmake(0.f, -0.045f),  4.f,  vmake(0.f,+0.25f), .1f,  8.f, 16.f,
	RGBA(255, 192, 128, 128), RGBA(0, 0, 0, 0)},

	/* smoke */{T_PARTICLE,  false,  2, 250, vmake(0.f,  0.05f) ,  4.f,  vmake(0.f, 0.f)  , .4f,  5.f, 12.f,
	RGBA(64,  64,  64, 192), RGBA(0, 0, 0, 0)},

	/* dust  */{T_PARTICLE,  false,  4, 100, vmake(0.f,  0.05f) ,  4.f,  vmake(0.f, 0.f)  , .4f,  3.f,  6.f,
	RGBA(192, 192, 192, 192), RGBA(0, 0, 0, 0)},

	/* gold  */{T_PARTICLE,  true ,  1,  50, vmake(0.f,  0.f)   ,  4.f,  vmake(0.f, 0.f)  , .3f,  3.f,  6.f,
	RGBA(192, 192,  64, 192), RGBA(0, 0, 0, 0)},
};

//=============================================================================
// Particle system model

static const size_t MAX_PSYSTEMS = 64;
static const size_t MAX_PARTICLES = 512;

struct Particle
{
	byte   active;
	byte   padding;
	word   age;
	vec2   pos;
	vec2   vel;
	float  radius;
	rgba   color;
};

struct PSystem
{
	PSType   type;
	vec2     source_pos;
	vec2     source_vel;
	Particle particles[MAX_PARTICLES];
};

PSystem psystems[MAX_PSYSTEMS];

//-----------------------------------------------------------------------------
void ResetPSystems()
{
	for (size_t i = 0; i < MAX_PSYSTEMS; i++)
		psystems[i].type = PST_NULL;
}

//-----------------------------------------------------------------------------
int CreatePSystem(PSType type, vec2 pos, vec2 vel)
{
	for (size_t i = 0; i < MAX_PSYSTEMS; i++)
	{
		if (psystems[i].type == PST_NULL)
		{
			psystems[i].type = type;
			psystems[i].source_pos = pos;
			psystems[i].source_vel = vel;
			for (size_t j = 0; j < MAX_PARTICLES; j++)
				psystems[i].particles[j].active = 0;
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
void KillPSystem(int index)
{
	if (index >= 0 && index < MAX_PSYSTEMS)
		psystems[index].type = PST_NULL;
}

//-----------------------------------------------------------------------------
void SetPSystemSource(int index, vec2 pos, vec2 vel)
{
	if (index >= 0 && index < MAX_PSYSTEMS)
	{
		psystems[index].source_pos = pos;
		psystems[index].source_vel = vel;
	}
}

//-----------------------------------------------------------------------------
void RenderPSystems(vec2 offset)
{
	glEnable(GL_BLEND);
	for (size_t i = 0; i < MAX_PSYSTEMS; i++)
	{
		if (psystems[i].type != PST_NULL)
		{
			if (psdefs[psystems[i].type].additive)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			else
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glBindTexture(GL_TEXTURE_2D, CORE_GetBmpOpenGLTex(Tex(psdefs[psystems[i].type].texture)));
			glBegin(GL_QUADS);

			for (size_t j = 0; j < MAX_PARTICLES; j++)
			{
				if (psystems[i].particles[j].active)
				{
					vec2 pos = psystems[i].particles[j].pos;
					float radius = psystems[i].particles[j].radius;
					vec2 p0 = vsub(pos, vmake(radius, radius));
					vec2 p1 = vadd(pos, vmake(radius, radius));
					rgba color = psystems[i].particles[j].color;
					float r = color.r;
					float g = color.g;
					float b = color.b;
					float a = color.a;

					glColor4f(r, g, b, a);

					glTexCoord2d(0.0, 0.0);
					glVertex2f(p0.x + offset.x, p0.y + offset.y);

					glTexCoord2d(1.0, 0.0);
					glVertex2f(p1.x + offset.x, p0.y + offset.y);

					glTexCoord2d(1.0, 1.0);
					glVertex2f(p1.x + offset.x, p1.y + offset.y);

					glTexCoord2d(0.0, 1.0);
					glVertex2f(p0.x + offset.x, p1.y + offset.y);

				}
			}

			glEnd();
		}
	}

	glColor4f(1.f, 1.f, 1.f, 1.);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//-----------------------------------------------------------------------------
void RunPSystems()
{
	for (size_t i = 0; i < MAX_PSYSTEMS; i++)
	{
		if (psystems[i].type != PST_NULL)
		{
			// New particles -----------------------------------------------------------------------
			for (size_t j = 0; j < psdefs[psystems[i].type].new_parts_per_frame; j++)
			{
				for (size_t k = 0; k < MAX_PARTICLES; k++)
				{
					if (!psystems[i].particles[k].active)
					{
						psystems[i].particles[k].active = 1;
						psystems[i].particles[k].age = 0;
						psystems[i].particles[k].pos =
							vadd(psystems[i].source_pos,
								vmake(CORE_FRand(-psdefs[psystems[i].type].start_pos_random, +psdefs[psystems[i].type].start_pos_random)
								, CORE_FRand(-psdefs[psystems[i].type].start_pos_random, +psdefs[psystems[i].type].start_pos_random)
							)
							);
						psystems[i].particles[k].vel =
							vadd(psystems[i].source_vel,
								vmake(
								psdefs[psystems[i].type].start_speed_fixed.x + CORE_FRand(-psdefs[psystems[i].type].start_speed_random, +psdefs[psystems[i].type].start_speed_random),
								psdefs[psystems[i].type].start_speed_fixed.y + CORE_FRand(-psdefs[psystems[i].type].start_speed_random, +psdefs[psystems[i].type].start_speed_random)
							)
							);
						psystems[i].particles[k].radius = CORE_FRand(psdefs[psystems[i].type].start_radius_min, psdefs[psystems[i].type].startradius_max);
						psystems[i].particles[k].color = psdefs[psystems[i].type].start_color_fixed;
						break;
					}
				}
			}

			// Run particles -----------------------------------------------------------------------
			for (size_t k = 0; k < MAX_PARTICLES; k++)
			{
				if (psystems[i].particles[k].active)
				{
					psystems[i].particles[k].age++;
					if (psystems[i].particles[k].age > psdefs[psystems[i].type].death)
						psystems[i].particles[k].active = 0;
					else
					{
						// Move
						psystems[i].particles[k].pos = vadd(psystems[i].particles[k].pos, psystems[i].particles[k].vel);
						psystems[i].particles[k].vel = vadd(psystems[i].particles[k].vel, psdefs[psystems[i].type].force);

						// Color
						psystems[i].particles[k].color.a = (psdefs[psystems[i].type].death - psystems[i].particles[k].age) / 255.f;
					}
				}
			}
		}
	}
}

//=============================================================================
// Game

TexId g_active_tileset = T_TILES_G_ON_S;
int   g_current_level = 0; // 0 to 8

//=============================================================================
// Levels
static const size_t NUM_LEVELS = 9;
struct LevelDesc
{
	TexId tileset;
	float level_length;
	float path_width;
	float path_twist_ratio;
	int   min_layers;
	int   max_layers;
	int   min_rocks_per_layer;
	int   max_rocks_per_layer;
	float chance_mine;
	float chance_drone;
	float min_space_between_layers;
	float max_space_between_layers;
	float min_space_between_challenges;
	float max_space_between_challenges;
	float level_start_chance_alt_terrain;
	float level_end_chance_alt_terrain;
};

LevelDesc LevelDescs[NUM_LEVELS] = {
	//
	{T_TILES_G_ON_S,  40000.f, (2.2f * MAINSHIP_RADIUS), 0.5f, 0, 10, 0, 3, 0.1f, 0.0f, 300.f, 600.f, 700.f, 2500.f, 0.2f, 0.9f},
	{T_TILES_P_ON_S,  60000.f, (2.1f * MAINSHIP_RADIUS), 0.5f, 1, 15, 0, 3, 0.1f, 0.0f, 300.f, 600.f, 650.f, 2300.f, 0.2f, 0.9f},
	{T_TILES_D_ON_S,  80000.f, (2.0f * MAINSHIP_RADIUS), 0.5f, 1, 20, 0, 3, 0.2f, 0.0f, 250.f, 600.f, 600.f, 2100.f, 0.2f, 0.9f},
	{T_TILES_E_ON_D, 100000.f, (1.9f * MAINSHIP_RADIUS), 0.6f, 2, 25, 1, 4, 0.3f, 0.0f, 250.f, 500.f, 550.f, 1900.f, 0.2f, 0.9f},
	{T_TILES_I_ON_W, 120000.f, (1.8f * MAINSHIP_RADIUS), 0.7f, 2, 30, 1, 4, 0.4f, 0.1f, 200.f, 500.f, 500.f, 1700.f, 0.1f, 0.9f},
	{T_TILES_Y_ON_X, 140000.f, (1.7f * MAINSHIP_RADIUS), 0.8f, 3, 35, 1, 4, 0.4f, 0.1f, 200.f, 500.f, 450.f, 1500.f, 0.2f, 0.9f},
	{T_TILES_R_ON_L, 160000.f, (1.6f * MAINSHIP_RADIUS), 0.9f, 3, 40, 2, 5, 0.5f, 0.1f, 200.f, 400.f, 400.f, 1300.f, 0.8f, 0.1f},
	{T_TILES_V_ON_K, 180000.f, (1.5f * MAINSHIP_RADIUS), 1.0f, 4, 45, 2, 5, 0.6f, 0.2f, 150.f, 400.f, 350.f, 1100.f, 0.2f, 0.9f},
	{T_TILES_B_ON_K, 200000.f, (1.4f * MAINSHIP_RADIUS), 1.1f, 4, 50, 2, 5, 0.7f, 0.3f, 150.f, 400.f, 300.f,  900.f, 0.2f, 0.9f}
};

//-----------------------------------------------------------------------------
// Background generation
static const int TILE_WIDTH   = 120;//96//75
static const int TILE_HEIGHT  = 140;//112//58
static const int TILES_ACROSS = (1+(int)((G_WIDTH +TILE_WIDTH -1)/TILE_WIDTH));
static const int TILES_DOWN   = (1+(int)((G_HEIGHT+TILE_HEIGHT-1)/TILE_HEIGHT));
static const int RUNNING_ROWS = (2 * TILES_DOWN);
// Bottom to top!
byte  Terrain[RUNNING_ROWS][TILES_ACROSS];
TexId TileMap[RUNNING_ROWS][TILES_ACROSS];

int g_last_generated = -1;

void GenTerrain(float upto)
{
	int last_required_row = 1 + (int)(upto / TILE_HEIGHT);

	// Generate random terrain types
	for (int i = g_last_generated + 1; i <= last_required_row; i++)
	{
		float advance = (i * TILE_HEIGHT) / LevelDescs[g_current_level].level_length;
		float chance = LevelDescs[g_current_level].level_start_chance_alt_terrain
			+ advance * (LevelDescs[g_current_level].level_end_chance_alt_terrain
				- LevelDescs[g_current_level].level_start_chance_alt_terrain);

		int mapped_row = UMod(i, RUNNING_ROWS);

		for (int j = 0; j < TILES_ACROSS; j++)
			Terrain[mapped_row][j] = (CORE_RandChance(chance) & 1);
	}

	// Calculate the tiles
	for (int i = g_last_generated; i <= last_required_row; i++)
	{
		int mapped_row = UMod(i, RUNNING_ROWS);

		for (int j = 0; j < TILES_ACROSS; j++)
		{
			unsigned v = (Terrain[mapped_row][j] << 1)
				| (Terrain[mapped_row][UMod(j + 1, TILES_ACROSS)] << 0)
				| (Terrain[UMod(mapped_row + 1, RUNNING_ROWS)][j] << 3)
				| (Terrain[UMod(mapped_row + 1, RUNNING_ROWS)][UMod(j + 1, TILES_ACROSS)] << 2);
			if (v > 15) v = 15;
			TileMap[mapped_row][j] = (TexId)(g_active_tileset + v);
		}
	}

	g_last_generated = last_required_row;
}

//-----------------------------------------------------------------------------
void RenderTerrain()
{
	int first_row = (int)(g_camera_offset / TILE_HEIGHT);

	for (int i = first_row; i < first_row + TILES_DOWN; i++)
	{
		int mapped_row = UMod(i, RUNNING_ROWS);
		for (int j = 0; j < TILES_ACROSS; j++)
			CORE_RenderCenteredSprite(
				vsub(vmake(j * TILE_WIDTH + .5f * TILE_WIDTH, i * TILE_HEIGHT + .5f * TILE_HEIGHT), vmake(0.f, g_camera_offset)),
				vmake(TILE_WIDTH * 1.01f, TILE_HEIGHT * 1.01f),
				Tex(TileMap[mapped_row][j])
			);
	}
}

//=============================================================================
// Entities - Custom struct to define objects in the world
enum EType { E_NULL, E_MAIN, E_ROCK, E_STAR, E_JUICE, E_MINE, E_DRONE, E_ROCKET };
static const size_t MAX_ENTITIES = 64;

struct Entity
{
	EType	type;
	vec2	pos;
	vec2	vel;
	float	radius;
	float	energy;
	float	fuel;
	float	tilt;
	float	tex_scale;
	TexId	texture;
	bool	tex_additive;
	bool	has_shadow;
	rgba	color;
	int		psystem;
	vec2	psystem_off;
};
Entity   g_entities[MAX_ENTITIES];
static Entity *MAIN_SHIP = &g_entities[MAINSHIP_ENTITY];

//-----------------------------------------------------------------------------
int InsertEntity(EType type, vec2 pos, vec2 vel, float radius, TexId tex,
	bool has_shadow, bool additive = false)
{
	for (size_t i = 0; i < MAX_ENTITIES; i++)
	{
		if (g_entities[i].type == E_NULL)
		{
			g_entities[i].type = type;
			g_entities[i].pos = pos;
			g_entities[i].vel = vel;
			g_entities[i].radius = radius;
			g_entities[i].energy = MAX_ENERGY;
			g_entities[i].fuel = MAX_FUEL;
			g_entities[i].tilt = 0.f;
			g_entities[i].tex_scale = 1.f;
			g_entities[i].texture = tex;
			g_entities[i].tex_additive = additive;
			g_entities[i].has_shadow = has_shadow;
			g_entities[i].color = COLOR_WHITE;
			g_entities[i].psystem = -1;
			g_entities[i].psystem_off = vmake(0.f, 0.f);
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
void KillEntity(int index)
{
	if (index >= 0 && index <= MAX_ENTITIES)
	{
		g_entities[index].type = E_NULL;
		if (g_entities[index].psystem >= 0)
			KillPSystem(g_entities[index].psystem);
	}
}

//-----------------------------------------------------------------------------
// Generate the level obstacles
void GenNextElements()
{
	// Called every game loop, but only does work when we are close to the next "challenge area"
	if (g_current_race_pos + G_HEIGHT > g_next_challenge_area)
	{
		// Get params from current level
		float path_width = LevelDescs[g_current_level].path_width;
		float path_twist_ratio = LevelDescs[g_current_level].path_twist_ratio;
		int   min_layers = LevelDescs[g_current_level].min_layers;
		int   max_layers = LevelDescs[g_current_level].max_layers;
		int   min_rocks_per_layer = LevelDescs[g_current_level].min_rocks_per_layer;
		int   max_rocks_per_layer = LevelDescs[g_current_level].max_rocks_per_layer;
		float chance_mine = LevelDescs[g_current_level].chance_mine;
		float chance_drone = LevelDescs[g_current_level].chance_drone;
		float min_space_between_layers = LevelDescs[g_current_level].min_space_between_layers;
		float max_space_between_layers = LevelDescs[g_current_level].max_space_between_layers;
		float min_space_between_challenges = LevelDescs[g_current_level].min_space_between_challenges;
		float max_space_between_challenges = LevelDescs[g_current_level].max_space_between_challenges;

		float current_y = g_next_challenge_area;
		LOG(("Current: %f\n", g_next_challenge_area));

		// Choose how many layers of rocks
		size_t nlayers = (int)CORE_URand(1, 20);
		LOG((" nlayers: %d\n", nlayers));
		for (size_t i = 0; i < nlayers; i++)
		{
			LOG(("  where: %f\n", current_y));

			// Choose pass point
			float displace = (current_y - g_last_conditioned.y) * path_twist_ratio;
			float bracket_left = g_last_conditioned.x - displace;
			float bracket_right = g_last_conditioned.x + displace;
			bracket_left = Max(bracket_left, 2.f * MAINSHIP_RADIUS);
			bracket_right = Max(bracket_right, G_WIDTH - 2.f * MAINSHIP_RADIUS);
			g_last_conditioned.y = current_y;
			g_last_conditioned.x = CORE_FRand(bracket_left, bracket_right);

			// Choose how many rocks
			size_t nrocks = (int)CORE_URand(1, 2);
			LOG(("  nrocks: %d\n", nrocks));

			// Gen rocks
			for (size_t i = 0; i < nrocks; i++)
			{
				// Find a valid pos
				vec2 rock_pos;
				while (true)
				{
					rock_pos = vmake(CORE_FRand(0.f, G_WIDTH), current_y);
					if (rock_pos.x + ROCK_RADIUS < g_last_conditioned.x - path_width ||
						rock_pos.x - ROCK_RADIUS > g_last_conditioned.x + path_width)
						break;
				}

				// Insert obstacle
				EType t = E_ROCK;
				TexId tex = T_ROCK1;
				if (CORE_RandChance(0.1f)) { t = E_MINE;  tex = T_MINE; }
				else if (CORE_RandChance(0.1f)) { t = E_DRONE; tex = T_DRONE2; }

				InsertEntity(t, rock_pos, vmake(CORE_FRand(-.5f, +.5f), CORE_FRand(-.5f, +.5f)),
					ROCK_RADIUS, tex, true);
			}

			current_y += CORE_FRand(300.f, 600.f);
		}

		g_next_challenge_area = current_y + CORE_FRand(.5f * G_HEIGHT, 1.5f * G_HEIGHT);
		LOG(("Next: %f\n\n", g_next_challenge_area));
	}
}

//-----------------------------------------------------------------------------
void Render()
{
	glClear(GL_COLOR_BUFFER_BIT);

	// Generate terrain as necessary for rendering next!
	GenTerrain(g_camera_offset + G_HEIGHT);
	RenderTerrain();

	// Draw entities (Reverse order to draw ship on top always)
	for (int i = MAX_ENTITIES - 1; i >= 0; i--)
	{
		if (g_entities[i].type != E_NULL)
		{
			ivec2 size = CORE_GetBmpSize(Tex(g_entities[i].texture));
			vec2 pos = g_entities[i].pos;
			pos.x = (float)((int)pos.x);
			pos.y = (float)((int)pos.y);

			// Draw shadow first if valid
			if (g_entities[i].has_shadow)
				CORE_RenderCenteredSprite(vadd(vsub(pos, vmake(0.f, g_camera_offset)),
					vmake(0.f, -SHADOW_OFFSET)), vmake(size.x * SPRITE_SCALE * g_entities[i].tex_scale
					* SHADOW_SCALE, size.y * SPRITE_SCALE * g_entities[i].tex_scale * SHADOW_SCALE),
					Tex(g_entities[i].texture), MakeRGBA(0.f, 0.f, 0.f, 0.4f), g_entities[i].tex_additive);

			// Draw actual entity
			CORE_RenderCenteredSprite(vsub(pos, vmake(0.f, g_camera_offset)),
				vmake(size.x * SPRITE_SCALE * g_entities[i].tex_scale, size.y * SPRITE_SCALE * g_entities[i].tex_scale), Tex(g_entities[i].texture), g_entities[i].color, g_entities[i].tex_additive);
		}
	}

	// Particle Systems
	RenderPSystems(vmake(0.f, -g_camera_offset));

	// Draw the UI
	if (g_gs != GS_VICTORY)
	{
		// Energy bar
		float energy_ratio = MAIN_SHIP->energy / MAX_ENERGY;
		CORE_RenderCenteredSprite(
			vmake(ENERGY_BAR_W / 2.f, energy_ratio * ENERGY_BAR_H / 2.f),
			vmake(ENERGY_BAR_W, ENERGY_BAR_H * energy_ratio),
			Tex(T_ENERGY), COLOR_WHITE, true);

		// Fuel bar
		float fuel_ratio = MAIN_SHIP->fuel / MAX_FUEL;
		CORE_RenderCenteredSprite(
			vmake(G_WIDTH - FUEL_BAR_W / 2.f, fuel_ratio * FUEL_BAR_H / 2.f),
			vmake(FUEL_BAR_W, FUEL_BAR_H * fuel_ratio),
			Tex(T_FUEL), COLOR_WHITE, true);

		// Pearl
		int num_chunks = (int)((g_current_race_pos / RACE_END) * MAX_CHUNKS);
		for (int i = 0; i < num_chunks; i++)
			CORE_RenderCenteredSprite(
				vmake(G_WIDTH - 100.f, 50.f + i * 50.f),
				vmake(CHUNK_W, CHUNK_H),
				Tex(T_PEARL));
	}
}

//-----------------------------------------------------------------------------
void ResetNewGame(int level)
{
	if (level < 0) level = 0;
	else if (level >= NUM_LEVELS) level = NUM_LEVELS - 1;
	g_current_level = level;
	g_active_tileset = LevelDescs[level].tileset;

	// Reset everything for a new game...
	g_next_challenge_area = FIRST_CHALLENGE;
	g_last_conditioned = vmake(.5f * G_WIDTH, 0.f);
	g_current_race_pos = 0.f;
	g_camera_offset = 0.f;
	g_rock_chance = START_ROCK_CHANCE_PER_PIXEL;
	g_gs = GS_START;
	g_gs_timer = 0.f;
	g_last_generated = -1;

	// Start logic
	for (size_t i = 0; i < MAX_ENTITIES; i++)
		g_entities[i].type = E_NULL;

	// Initialize main ship
	InsertEntity(E_MAIN, vmake(G_WIDTH / 2.0, G_HEIGHT / 8.f), vmake(0.f, SHIP_START_SPEED), MAINSHIP_RADIUS, T_SHIP_C, true);

	PlayLoopSound(1, SND_ENGINE, 0.7f, 0.3f);

	ResetPSystems();

	MAIN_SHIP->psystem = CreatePSystem(PST_FIRE, MAIN_SHIP->pos, vmake(0.f, 0.f));
	MAIN_SHIP->psystem_off = vmake(0.f, -120.f);
}

//-----------------------------------------------------------------------------
void RunGame()
{
	// Control main ship
	if (g_gs == GS_PLAYING || g_gs == GS_VICTORY)
	{
		if (MAIN_SHIP->vel.y < SHIP_CRUISE_SPEED)
		{
			MAIN_SHIP->vel.y = SafeAdd(MAIN_SHIP->vel.y, SHIP_INC_SPEED, SHIP_CRUISE_SPEED);
		}

		MAIN_SHIP->fuel = SafeSub(MAIN_SHIP->fuel, FRAME_FUEL_COST);
	}

	// Heal main ship
	if (g_gs != GS_DYING)
	{
		if (MAIN_SHIP->energy < MAX_ENERGY && MAIN_SHIP->fuel >= MIN_FUEL_FOR_HEAL)
		{
			MAIN_SHIP->energy = SafeAdd(MAIN_SHIP->energy, ENERGY_HEAL_PER_FRAME, MAX_ENERGY);
			MAIN_SHIP->fuel = SafeSub(MAIN_SHIP->fuel, FUEL_HEAL_PER_FRAME);
		}
	}

	// Move entities
	for (size_t i = 0; i < MAX_ENTITIES; i++)
	{
		if (g_entities[i].type != E_NULL)
		{
			g_entities[i].pos = vadd(g_entities[i].pos, g_entities[i].vel);

			// Remove entities that fell off the screen
			if (g_entities[i].pos.y < g_camera_offset - G_HEIGHT)
				KillEntity(i);

			if (g_entities[i].psystem != -1)
				SetPSystemSource(g_entities[i].psystem, vadd(g_entities[i].pos, g_entities[i].psystem_off), g_entities[i].vel);
		}

		// Advance 'stars'
		if (g_entities[i].type == E_STAR)
		{
			g_entities[i].tex_scale *= 1.008f;
		}
	}

	// Advance particle systems
	RunPSystems();

	// Dont let steering off the screen!
	if (g_entities[MAINSHIP_ENTITY].pos.x < MAINSHIP_RADIUS)
		g_entities[MAINSHIP_ENTITY].pos.x = MAINSHIP_RADIUS;
	if (g_entities[MAINSHIP_ENTITY].pos.x > G_WIDTH - MAINSHIP_RADIUS)
		g_entities[MAINSHIP_ENTITY].pos.x = G_WIDTH - MAINSHIP_RADIUS;

	// Check collisions between main ship and rocks
	if (g_gs == GS_PLAYING)
	{
		// Check everything against the ship
		for (int i = 1; i < MAX_ENTITIES; i++)
		{
			// Collision matrix for the entities wrt ship
			if (g_entities[i].type == E_ROCK ||
				g_entities[i].type == E_JUICE ||
				g_entities[i].type == E_MINE ||
				g_entities[i].type == E_DRONE)
			{
				if (vlen2(vsub(g_entities[i].pos, MAIN_SHIP->pos)) < CORE_FSquare(g_entities[i].radius + MAIN_SHIP->radius))
				{
					switch (g_entities[i].type)
					{
					case E_ROCK:
						if (g_entities[i].energy > 0)
						{
							PlaySound(SND_THUMP);
							MAIN_SHIP->energy = SafeSub(MAIN_SHIP->energy, ROCK_CRASH_ENERGY_LOSS);
							MAIN_SHIP->vel.y = SHIP_START_SPEED;
							g_entities[i].vel = vscale(vunit(vsub(g_entities[i].pos, MAIN_SHIP->pos)), CRASH_VEL);
							g_entities[i].energy = 0;
						}
						break;

					case E_JUICE:
						MAIN_SHIP->fuel = SafeAdd(MAIN_SHIP->fuel, JUICE_FUEL, MAX_FUEL);
						KillEntity(i);
						break;

					case E_MINE:
						PlaySound(SND_EXPLOSION);
						MAIN_SHIP->energy = SafeSub(MAIN_SHIP->energy, MINE_CRASH_ENERGY_LOSS);
						MAIN_SHIP->vel.y = SHIP_START_SPEED;
						KillEntity(i);
						break;

					case E_DRONE:
						MAIN_SHIP->energy = SafeSub(MAIN_SHIP->energy, MINE_CRASH_ENERGY_LOSS);
						MAIN_SHIP->vel.y = SHIP_START_SPEED;
						KillEntity(i);
						break;

					default:
						break;
					}
				}
			}
			else if (g_entities[i].type == E_ROCKET)
			{
				// Check all rocks against this rocket
				for (size_t j = 1; j < MAX_ENTITIES; j++)
				{
					// Should check against ship?
					if (g_entities[j].type == E_ROCK
						|| g_entities[j].type == E_MINE
						|| g_entities[j].type == E_DRONE
						)
					{
						if (vlen2(vsub(g_entities[i].pos, g_entities[j].pos)) < CORE_FSquare(g_entities[i].radius + g_entities[j].radius))
						{
							// Rocket hit the target!
							switch (g_entities[j].type)
							{
							case E_MINE:
								PlaySound(SND_EXPLOSION);
								break;
							default:
								break;
							}

							KillEntity(i);
							KillEntity(j);

							break; // Stop checking rocks!
						}
					}
				}
			}
		}
	}

	// Generate new level elements as we advance
	GenNextElements();

	// Possibly insert juice
	if (g_gs == GS_PLAYING)
	{
		float trench = MAIN_SHIP->pos.y - g_current_race_pos; // How much advanced from previous frame
		if (CORE_RandChance(trench * JUICE_CHANCE_PER_PIXEL))
		{
			InsertEntity(E_JUICE,
				vmake(CORE_FRand(0.f, G_WIDTH), g_camera_offset + G_HEIGHT + GEN_IN_ADVANCE),
				vmake(CORE_FRand(-1.f, +1.f), CORE_FRand(-1.f, +1.f)),
				JUICE_RADIUS, T_JUICE, false, true);
		}
	}

	// Set camera to follow the main ship
	g_camera_offset = MAIN_SHIP->pos.y - G_HEIGHT / 8.f;

	g_current_race_pos = MAIN_SHIP->pos.y;
	// Check for end state
	if (g_gs == GS_PLAYING)
	{
		if (g_current_race_pos >= RACE_END)
		{
			g_gs = GS_VICTORY;
			g_gs_timer = 0.f;
			MAIN_SHIP->tex_additive = true;
			PlaySound(SND_SUCCESS);
		}
	}

	// Advance game mode
	g_gs_timer += FRAMETIME;
	switch (g_gs)
	{
	case GS_START:
		if (g_gs_timer >= STARTING_TIME)
		{
			g_gs = GS_PLAYING;
			g_gs_timer = 0.f;
		}
		break;

	case GS_DYING:
		if (g_gs_timer >= DYING_TIME)
		{
			ResetNewGame(g_current_level);
		}
		break;

	case GS_PLAYING:
		if (MAIN_SHIP->energy <= 0.f || MAIN_SHIP->fuel <= 0.f)
		{
			g_gs = GS_DYING;
			g_gs_timer = 0.f;
			MAIN_SHIP->texture = T_SHIP_RR;
		}
		break;

	case GS_VICTORY:
		if (CORE_RandChance(1.f / 10.f))
			InsertEntity(E_STAR,
				MAIN_SHIP->pos,
				vadd(MAIN_SHIP->vel, vmake(CORE_FRand(-5.f, 5.f), CORE_FRand(-5.f, 5.f))),
				0, T_STAR, false, true);
		if (g_gs_timer >= VICTORY_TIME)
			ResetNewGame(g_current_level + 1);
		break;
	}

	g_time_from_last_rocket += FRAMETIME;
}

//-----------------------------------------------------------------------------
void ProcessInput()
{
	if (g_gs == GS_PLAYING)
	{
		if (SYS_KeyPressed(' ') && g_time_from_last_rocket > MIN_TIME_BETWEEN_ROCKETS)
		{
			int e = InsertEntity(E_ROCKET, MAIN_SHIP->pos, vadd(MAIN_SHIP->vel, vmake(0.f, ROCKET_SPEED)),
				ROCKET_RADIUS, T_ROCKET, true);
			g_time_from_last_rocket = 0;

			g_entities[e].psystem = CreatePSystem(PST_FIRE, MAIN_SHIP->pos, vmake(0.f, 0.f));
			g_entities[e].psystem_off = vmake(0.f, -120.f);
		}

		bool up = SYS_KeyPressed(SYS_KEY_UP);
		bool down = SYS_KeyPressed(SYS_KEY_DOWN);
		bool left = SYS_KeyPressed(SYS_KEY_LEFT);
		bool right = SYS_KeyPressed(SYS_KEY_RIGHT);

		// Left-right movement
		if (left && !right)
		{
			MAIN_SHIP->fuel = SafeSub(MAIN_SHIP->fuel, TILT_FUEL_COST);
			MAIN_SHIP->tilt -= SHIP_TILT_INC;
		}
		if (right && !left)
		{
			MAIN_SHIP->fuel -= TILT_FUEL_COST;
			MAIN_SHIP->tilt += SHIP_TILT_INC;
		}
		if (!left && !right)
			MAIN_SHIP->tilt *= (1.f - SHIP_TILT_FRICTION);

		if (MAIN_SHIP->tilt <= -SHIP_MAX_TILT) MAIN_SHIP->tilt = -SHIP_MAX_TILT;
		if (MAIN_SHIP->tilt >= SHIP_MAX_TILT) MAIN_SHIP->tilt = SHIP_MAX_TILT;

		MAIN_SHIP->vel.x += MAIN_SHIP->tilt;
		MAIN_SHIP->vel.x *= (1.f - SHIP_HVEL_FRICTION);

		// Accelerate/slowdown
		if (up && !down) MAIN_SHIP->vel.y += SHIP_INC_SPEED;
		if (down && !up)   MAIN_SHIP->vel.y -= SHIP_INC_SPEED;
		if (MAIN_SHIP->vel.y > SHIP_MAX_SPEED) MAIN_SHIP->vel.y = SHIP_MAX_SPEED;
		if (MAIN_SHIP->vel.y < SHIP_MIN_SPEED) MAIN_SHIP->vel.y = SHIP_MIN_SPEED;

		float tilt = MAIN_SHIP->tilt;
		if (tilt < -.6f * SHIP_MAX_TILT)	  MAIN_SHIP->texture = T_SHIP_LL;
		else if (tilt < -.2f * SHIP_MAX_TILT) MAIN_SHIP->texture = T_SHIP_L;
		else if (tilt < +.2f * SHIP_MAX_TILT) MAIN_SHIP->texture = T_SHIP_C;
		else if (tilt < +.6f * SHIP_MAX_TILT) MAIN_SHIP->texture = T_SHIP_R;
		else                                  MAIN_SHIP->texture = T_SHIP_RR;
	}

	if (SYS_KeyPressed('1')) ResetNewGame(0);
	else if (SYS_KeyPressed('2')) ResetNewGame(1);
	else if (SYS_KeyPressed('3')) ResetNewGame(2);
	else if (SYS_KeyPressed('4')) ResetNewGame(3);
	else if (SYS_KeyPressed('5')) ResetNewGame(4);
	else if (SYS_KeyPressed('6')) ResetNewGame(5);
	else if (SYS_KeyPressed('7')) ResetNewGame(6);
	else if (SYS_KeyPressed('8')) ResetNewGame(7);
	else if (SYS_KeyPressed('9')) ResetNewGame(8);
}

//-----------------------------------------------------------------------------
// Game state (apart from entities & other stand-alone modules)
float g_time = 0.f;

//-----------------------------------------------------------------------------
// Main
int Main(void)
{
	// Start things up & load resources ---------------------------------------------------
	CORE_InitSound();
	LoadTextures();
	LoadSounds();
	ResetNewGame(0);

	// Set up rendering ---------------------------------------------------------------------
	glViewport(0, 0, SYS_WIDTH, SYS_HEIGHT);
	glClearColor(0.0f, 0.1f, 0.3f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, G_WIDTH, 0.0, G_HEIGHT, 0.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Main game loop! ======================================================================
	while (!SYS_GottaQuit())
	{
		Render();
		SYS_Show();
		ProcessInput();
		RunGame();
		SYS_Pump();
		SYS_Sleep(16);
		g_time += FRAMETIME;
	}

	UnloadSounds();
	UnloadTextures();
	CORE_EndSound();

	return 0;
}