/*
 * core.cpp - Implementation of the core texture handling functions
 */
#include "stdafx.h"
#include "base.h"
#include "core.h"

//=============================================================================
// Loading textures (from BMP files)

static const size_t MAX_TEXTURES = 256;
struct Texture
{
	bool used;
	float w, h;			// In 0..1 terms
	int pix_w, pix_h;	// In pixels
	GLuint tex;
} g_textures[MAX_TEXTURES] = {0};

//-----------------------------------------------------------------------------
struct CORE_BMPFileHeader
{
	byte  mark[2];
	byte  filesize[4];
	byte  reserved[4];
	byte  pixdataoffset[4];

	byte  hdrsize[4];
	byte  width[4];
	byte  height[4];
	byte  colorplanes[2];
	byte  bpp[2];
	byte  compression[4];
	byte  pixdatasize[4];
	byte  hres[4];
	byte  vres[4];
	byte  numcol[4];
	byte  numimportantcolors[4];
};

//-----------------------------------------------------------------------------
// Utility functions
template<typename T>
inline word ReadWord(const T a[])	{ return (a[0] + a[1]*0x100); }
template<typename T>
inline dword ReadDWord(const T a[]) { return (a[0] + a[1] * 0x100 + a[2] * 0x10000 + a[3] * 0x1000000); }

// Next higher power of 2
dword hp2(dword v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++; 
	return v += (v == 0);
}

// Pixel load buffer
static byte pixloadbuffer[2048 * 2048 * 4];

// Core functions
int CORE_LoadBmp(const char filename[], bool wrap)
{
	GLint	retval = -1;
	struct	CORE_BMPFileHeader hdr;

	int fd = open(filename, O_RDONLY);
	if ( fd != -1 )
	{
		read(fd, &hdr, sizeof(hdr));

		if ( hdr.mark[0] == 'B' && hdr.mark[1] == 'M' )
		{
			// Find an empty texture entry
			for ( int i = 0; i < MAX_TEXTURES; i++ )
			{
				if ( !g_textures[i].used )
				{
					retval = i;
					break;
				}
			}

			if ( retval == -1 )
				return retval;

			dword  width = ReadDWord(hdr.width);
			sdword height = ReadDWord(hdr.height);
			dword  offset = ReadDWord(hdr.pixdataoffset);

			dword pixdatasize = ReadDWord(hdr.pixdatasize);
			if ( !pixdatasize )
				pixdatasize = (width * abs(height) * ReadWord(hdr.bpp) / 8);

			lseek(fd, offset, SEEK_SET);
			if ( height > 0 )
				read(fd, pixloadbuffer, pixdatasize);
			else
			{
				// Reverse while loading
				int nrows = -height;
				for ( int i = 0; i < nrows; i++ )
					read(fd, pixloadbuffer + (nrows - i - 1) * width * 4, (pixdatasize / nrows));
			}

			GLuint texid = 1;

			glGenTextures(1, &texid);
			glBindTexture(GL_TEXTURE_2D, texid);
			//glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // GL_LINEAR_MIPMAP_NEAREST
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP);

			//gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGBA8, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixloadbuffer );

			height = abs((int)height);
			dword width_pow2 = hp2(width);
			dword height_pow2 = hp2(height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_pow2, height_pow2, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixloadbuffer);

			g_textures[retval].used = true;
			g_textures[retval].tex = texid;
			g_textures[retval].pix_w = width;
			g_textures[retval].pix_h = height;
			g_textures[retval].w = width / (float)width_pow2;
			g_textures[retval].h = height / (float)height_pow2;
		}
		close(fd);
	}

	return retval;
}
//-----------------------------------------------------------------------------
void CORE_UnloadBmp(int texture_index)
{
	glDeleteTextures(1, &g_textures[texture_index].tex);
	g_textures[texture_index].used = false;
}

//-----------------------------------------------------------------------------
ivec2 CORE_GetBmpSize(int texture_index)
{
	ivec2 v;
	v.x = g_textures[texture_index].pix_w;
	v.y = g_textures[texture_index].pix_h;
	return v;
}

//-----------------------------------------------------------------------------
GLuint CORE_GetBmpOpenGLTex(int texture_index)
{
	return g_textures[texture_index].tex;
}

//-----------------------------------------------------------------------------
void CORE_RenderCenteredSprite(vec2 pos, vec2 size, int texture_index, rgba color, bool additive)
{
	glColor4f(color.r, color.g, color.b, color.a);
	if (additive)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	else
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	vec2 p0 = vsub(pos, vscale(size, .5f));
	vec2 p1 = vadd(pos, vscale(size, .5f));

	glBindTexture(GL_TEXTURE_2D, g_textures[texture_index].tex);
	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 0.0);													glVertex2f(p0.x, p0.y);
	glTexCoord2d(g_textures[texture_index].w, 0.0);							glVertex2f(p1.x, p0.y);
	glTexCoord2d(g_textures[texture_index].w, g_textures[texture_index].h); glVertex2f(p1.x, p1.y);
	glTexCoord2d(0.0, g_textures[texture_index].h);							glVertex2f(p0.x, p1.y);
	glEnd();
}

//=============================================================================
// Sound (OpenAL, WAV files), leave last one for music!
static const size_t SND_MAX_SOURCES = 8;
static const size_t SND_MAX_LOOP_SOUNDS = 2;
ALuint SND_Sources[SND_MAX_SOURCES];
size_t SND_NextSource = 0;

//-----------------------------------------------------------------------------
bool CORE_InitSound()
{
	ALCcontext *context;
	ALCdevice * device;

	device = alcOpenDevice(NULL);
	if(device)
	{
		context = alcCreateContext(device, NULL);
		alcMakeContextCurrent(context);
		alGenSources(SND_MAX_SOURCES, SND_Sources);
		alGetError();
		return true;
	}
	else
		return false;
}

//-----------------------------------------------------------------------------
void CORE_EndSound()
{
	ALCdevice *device;
	ALCcontext *context;

	context = alcGetCurrentContext();
	device = alcGetContextsDevice(context);

	alDeleteSources(SND_MAX_SOURCES, SND_Sources);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

//-----------------------------------------------------------------------------
void CORE_PlaySound(ALuint snd, float volume, float pitch)
{
	// Stop and play!
	alSourceStop(SND_Sources[SND_NextSource]);
	alSourcei(SND_Sources[SND_NextSource], AL_BUFFER, snd);
	alSourcef(SND_Sources[SND_NextSource], AL_GAIN, volume);
	alSourcef(SND_Sources[SND_NextSource], AL_PITCH, pitch);
	alSourcePlay(SND_Sources[SND_NextSource]);

	SND_NextSource++;
	SND_NextSource = SND_NextSource % SND_MAX_LOOP_SOUNDS;
}

//-----------------------------------------------------------------------------
void CORE_PlayLoopSound(size_t loop_channel, ALuint snd, float volume, float pitch)
{
	if (loop_channel >= SND_MAX_LOOP_SOUNDS)
		return;
	alSourceStop(SND_Sources[SND_MAX_SOURCES - loop_channel]);
	alSourcei(SND_Sources[SND_MAX_SOURCES - loop_channel], AL_BUFFER, snd);
	alSourcef(SND_Sources[SND_MAX_SOURCES - loop_channel], AL_GAIN, volume);
	alSourcef(SND_Sources[SND_MAX_SOURCES - loop_channel], AL_PITCH, pitch);
	alSourcei(SND_Sources[SND_MAX_SOURCES - loop_channel], AL_LOOPING, AL_TRUE);
	alSourcePlay(SND_Sources[SND_MAX_SOURCES - loop_channel]);
}

//-----------------------------------------------------------------------------
void CORE_SetLoopSoundParam(size_t loop_channel, float volume, float pitch)
{
	if (loop_channel >= SND_MAX_LOOP_SOUNDS)
		return;
	alSourcef(SND_Sources[SND_MAX_SOURCES - loop_channel], AL_GAIN, volume);
	alSourcef(SND_Sources[SND_MAX_SOURCES - loop_channel], AL_PITCH, pitch);
}

//-----------------------------------------------------------------------------
void CORE_StopLoopSound(size_t loop_channel)
{
	alSourceStop(SND_Sources[SND_MAX_SOURCES - loop_channel]);
}

//-----------------------------------------------------------------------------
struct CORE_RIFFHeader
{
	byte  chunk_ID[4];   // 'RIFF'
	byte  chunk_size[4];
	byte  format[4];    // 'WAVE'
};

struct CORE_RIFFChunkHeader
{
	byte  sub_chunk_ID[4];
	byte  sub_chunk_size[4];
};

struct CORE_WAVEFormatChunk
{
	byte  audio_format[2];
	byte  num_channels[2];
	byte  sample_rate[4];
	byte  byte_rate[4];
	byte  block_alignp[2];
	byte  bits_per_sample[2];
};

//-----------------------------------------------------------------------------
static const size_t MAX_WAV_SIZE = 32*1024*1024; // Max 32Mb sound!
static byte sound_load_buffer[MAX_WAV_SIZE];

ALuint CORE_LoadWav(const char filename[])
{
	ALuint			retval = UINT_MAX;
	CORE_RIFFHeader hdr;

	int fd = open(filename, O_RDONLY);
	if (fd != 1)
	{
		read(fd, &hdr, sizeof(hdr));

		if (hdr.chunk_ID[0] == 'R' && hdr.chunk_ID[1] == 'I' && hdr.chunk_ID[2] == 'F' && hdr.chunk_ID[3] =='F'
			&& hdr.format[0] == 'W' && hdr.format[1] == 'A' && hdr.format[2] == 'V' && hdr.format[3] == 'E')
		{
			CORE_WAVEFormatChunk fmt;
			memset(&fmt, 0, sizeof(fmt));

			while(true)
			{
				CORE_RIFFChunkHeader chunk_hdr;
				if (read(fd, &chunk_hdr, sizeof(chunk_hdr)) < sizeof(chunk_hdr))
					break;

				dword chunk_data_size = ReadDWord(chunk_hdr.sub_chunk_size);
					
				if (chunk_hdr.sub_chunk_ID[0] == 'f' && chunk_hdr.sub_chunk_ID[1] == 'm' &&
					chunk_hdr.sub_chunk_ID[2] == 't' && chunk_hdr.sub_chunk_ID[3] == ' ')
				{
					read(fd, &fmt, sizeof(fmt));
					lseek(fd, ((1 + chunk_data_size) & -2) - sizeof(fmt), SEEK_CUR); // Skip to next chunk
				}
				else if (chunk_hdr.sub_chunk_ID[0] == 'd' && chunk_hdr.sub_chunk_ID[1] == 'a' &&
					chunk_hdr.sub_chunk_ID[2] == 't' && chunk_hdr.sub_chunk_ID[3] == 'a')
				{
					dword content_size = chunk_data_size;
					if (content_size > sizeof(sound_load_buffer))
						content_size = sizeof(sound_load_buffer);

					read(fd, sound_load_buffer, content_size);
					
					bool valid = true;
					ALsizei al_size = content_size;
					ALsizei al_frequency = ReadDWord(fmt.sample_rate);
					ALenum al_format = (ALenum)-1;
					if (ReadWord(fmt.num_channels) == 1)
					{
						if (ReadWord(fmt.bits_per_sample) == 8) al_format = AL_FORMAT_MONO8;
						else if (ReadWord(fmt.bits_per_sample) == 16) al_format = AL_FORMAT_MONO16;
						else valid = false;
					}
					else if (ReadWord(fmt.num_channels) == 2)
					{
						if (ReadWord(fmt.bits_per_sample) == 8) al_format = AL_FORMAT_STEREO8;
						else if (ReadWord(fmt.bits_per_sample) == 16) al_format = AL_FORMAT_STEREO16;
						else valid = false;
					}

					if (valid)
					{
						ALuint al_buffer = UINT_MAX;
						alGenBuffers(1, &al_buffer);

						alBufferData(al_buffer, al_format, sound_load_buffer, al_size, al_frequency);
						retval = al_buffer;
					}
					break;
				}
				else
				{
					lseek(fd, ((1 + ReadDWord(chunk_hdr.sub_chunk_size)) & -2), SEEK_CUR); //Skip to next chunk
				}
			}
		}
		close (fd);
	}
	return retval;
}

//-----------------------------------------------------------------------------
void CORE_UnloadWav(ALuint snd)
{
	alDeleteBuffers(1, &snd);
}