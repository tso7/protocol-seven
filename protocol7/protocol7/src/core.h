/*
 * core.h - Engine relevant modules: Texture/graphics functions
 */
#pragma once
#ifndef	P7_CORE_H_
#define P7_CORE_H_

//-----------------------------------------------------------------------------
// Random generation functions
inline float	CORE_FRand(float from, float to) { return from + (to - from) 
				* (rand() / (float)RAND_MAX); }
inline unsigned CORE_URand(unsigned from, unsigned to) { return from + rand() 
				% (unsigned)(to - from + 1); }
inline bool		CORE_RandChance(float chance) { return CORE_FRand(0.f, 1.f) < chance; }
inline float	CORE_FSquare(float f) { return f * f; }

//-----------------------------------------------------------------------------
inline int UMod(int index, int n) { return (int)((unsigned)index % (unsigned)n); }

//-----------------------------------------------------------------------------
// RGBA container
struct rgba	{ float r, g, b, a; };
inline rgba MakeRGBA(float r, float g, float b, float a) { rgba c; c.r = r; 
			c.g = g; c.b = b; c.a = a; return c; }
inline rgba RGBA(float rr, float gg, float bb, float aa) { return MakeRGBA(rr/255.f,
			gg/255.f, bb/255.f, aa/255.f);}

static const rgba COLOR_WHITE = MakeRGBA(1.f, 1.f, 1.f, 1.f);

//-----------------------------------------------------------------------------
// Bitmap/texture functions
int		CORE_LoadBmp(const char filename[], bool wrap);
ivec2	CORE_GetBmpSize(int texture_index);
GLuint	CORE_GetBmpOpenGLTex(int texture_index);
void	CORE_UnloadBmp(int texture_index);
void	CORE_RenderCenteredSprite(vec2 pos, vec2 size, int texture_index, 
			rgba color = COLOR_WHITE, bool additive = false);

//-----------------------------------------------------------------------------
// Sound
bool CORE_InitSound();
void CORE_EndSound();
uint CORE_LoadWav(const char filename[]);
void CORE_UnloadWav(ALuint snd);
void CORE_PlaySound(ALuint snd, float volume, float pitch);
void CORE_PlayLoopSound(size_t loop_channel, ALuint snd, float volume, float pitch);
void CORE_SetLoopSoundParam(size_t loop_channel, float volume, float pitch);
void CORE_StopLoopSound(size_t loop_channel);

#endif // !P7_CORE_H_