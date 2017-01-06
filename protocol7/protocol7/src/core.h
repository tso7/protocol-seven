/*
 * core.h - Engine relevant modules: Texture/graphics functions
 */
#pragma once
#ifndef	P7_CORE_H_
#define P7_CORE_H_

// Bitmap/texture functions
int		CORE_LoadBmp(const char filename[], const bool &wrap);
ivec2	CORE_GetBmpSize(const int texture_index);
void	CORE_UnloadBmp(const int texture_index);
void	CORE_RenderCenteredSprite(vec2 pos, vec2 size, int texture_index);

#endif // !P7_CORE_H_