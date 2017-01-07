/*
 * game2.cpp - Added rocks and endless scrolling
 */
#include "stdafx.h"
#include "base.h"
#include "sys.h"
#include "core.h"

namespace Game2
{
	//=============================================================================
	// Game Parameter Constants
	static const float SPRITE_SCALE = 8.f;
	static const size_t MAINSHIP_ENTITY = 0;
	static const float MAINSHIP_RADIUS = 100.f;
	static const float ROCK_RADIUS = 100.f;
	static const float SHIP_W = 250;
	static const float SHIP_H = 270;
	static const float VERTICAL_SHIP_VEL = 20.f;
	static const float HORIZONTAL_SHIP_VEL = 10.f;

	float g_camera_offset = 0.f;

	//=============================================================================
	// Textures
	int g_ship_LL, g_ship_L, g_ship_C, g_ship_R, g_ship_RR;
	int g_bkg;
	int g_rock[5];

	//=============================================================================
	// Entities - Custom struct to define objects in the world

	enum EType { E_NULL, E_MAIN, E_ROCK };

	static const size_t MAX_ENTITIES = 64;

	struct Entity
	{
		EType  type;
		vec2   pos;
		vec2   vel;
		float  radius;
		int    texture;
	};
	Entity   g_entities[MAX_ENTITIES];

	//-----------------------------------------------------------------------------
	void InsertEntity(EType type, vec2 pos, vec2 vel, float radius, int tex)
	{
		for (int i = 0; i < MAX_ENTITIES; i++)
		{
			if (g_entities[i].type == E_NULL)
			{
				g_entities[i].type = type;
				g_entities[i].pos = pos;
				g_entities[i].vel = vel;
				g_entities[i].radius = radius;
				g_entities[i].texture = tex;
				break;
			}
		}
	}
	//-----------------------------------------------------------------------------
	void StartGame()
	{
		// Resources
		g_ship_LL = CORE_LoadBmp("data/ShipLL.bmp", false);
		g_ship_L = CORE_LoadBmp("data/ShipL.bmp", false);
		g_ship_C = CORE_LoadBmp("data/ShipC.bmp", false);
		g_ship_R = CORE_LoadBmp("data/ShipR.bmp", false);
		g_ship_RR = CORE_LoadBmp("data/ShipRR.bmp", false);
		g_bkg = CORE_LoadBmp("data/bkg0.bmp", false);
		g_rock[0] = CORE_LoadBmp("data/Rock0.bmp", false);
		g_rock[1] = CORE_LoadBmp("data/Rock1.bmp", false);
		g_rock[2] = CORE_LoadBmp("data/Rock2.bmp", false);
		g_rock[3] = CORE_LoadBmp("data/Rock3.bmp", false);
		g_rock[4] = CORE_LoadBmp("data/Rock4.bmp", false);

		// Initialize main ship
		InsertEntity(E_MAIN, vmake(G_WIDTH / 2.0, G_HEIGHT / 8.f), vmake(0.f, VERTICAL_SHIP_VEL), MAINSHIP_RADIUS, g_ship_C);
	}

	//-----------------------------------------------------------------------------
	void EndGame()
	{
		CORE_UnloadBmp(g_ship_LL);
		CORE_UnloadBmp(g_ship_L);
		CORE_UnloadBmp(g_ship_C);
		CORE_UnloadBmp(g_ship_R);
		CORE_UnloadBmp(g_ship_RR);
		CORE_UnloadBmp(g_bkg);
		CORE_UnloadBmp(g_rock[0]);
		CORE_UnloadBmp(g_rock[1]);
		CORE_UnloadBmp(g_rock[2]);
		CORE_UnloadBmp(g_rock[3]);
		CORE_UnloadBmp(g_rock[4]);
	}

	//-----------------------------------------------------------------------------
	void Render()
	{
		glClear(GL_COLOR_BUFFER_BIT);

		// Render background, only tiles within the camera view
		size_t first_tile = (size_t)floorf(g_camera_offset / G_HEIGHT);
		for (size_t i = first_tile; i < first_tile + 2; i++)
		{
			CORE_RenderCenteredSprite(vsub(vadd(vmake(G_WIDTH / 2.0f, G_HEIGHT / 2.0f),
				vmake(0.f, (float)i * G_HEIGHT)), vmake(0.f, g_camera_offset)), vmake(G_WIDTH, G_HEIGHT),
				g_bkg);
		}

		// Draw entities (Reverse order to draw ship on top always)
		for (int i = MAX_ENTITIES - 1; i >= 0; i--)
		{
			if (g_entities[i].type != E_NULL)
			{
				ivec2 size = CORE_GetBmpSize(g_entities[i].texture);
				vec2 pos = g_entities[i].pos;
				pos.x = (float)((int)pos.x);
				pos.y = (float)((int)pos.y);
				CORE_RenderCenteredSprite(vsub(pos, vmake(0.f, g_camera_offset)),
					vmake(size.x * SPRITE_SCALE, size.y * SPRITE_SCALE), g_entities[i].texture);
			}
		}
	}

	//-----------------------------------------------------------------------------
	void RunGame()
	{
		// Move entities
		for (size_t i = 0; i < MAX_ENTITIES; i++)
		{
			if (g_entities[i].type != E_NULL)
			{
				g_entities[i].pos = vadd(g_entities[i].pos, g_entities[i].vel);

				// Remove entities that fell off the screen
				if (g_entities[i].pos.y < g_camera_offset - G_HEIGHT)
					g_entities[i].type = E_NULL;
			}
		}

		// Dont let steering off the screen!
		if (g_entities[MAINSHIP_ENTITY].pos.x < MAINSHIP_RADIUS)
			g_entities[MAINSHIP_ENTITY].pos.x = MAINSHIP_RADIUS;
		if (g_entities[MAINSHIP_ENTITY].pos.x > G_WIDTH - MAINSHIP_RADIUS)
			g_entities[MAINSHIP_ENTITY].pos.x = G_WIDTH - MAINSHIP_RADIUS;

		// Possibly insert new rock
		if (rand() < (RAND_MAX / 40))
			InsertEntity(E_ROCK,
				vmake((rand() * (float)G_WIDTH) / RAND_MAX, g_camera_offset + G_HEIGHT + 400.f),
				vmake(0.f, 0.f), ROCK_RADIUS, g_rock[rand() % 5U]);

		// Set camera to follow the main ship
		g_camera_offset = g_entities[MAINSHIP_ENTITY].pos.y - G_HEIGHT / 8.f;
	}

	//-----------------------------------------------------------------------------
	void ProcessInput()
	{
		if (SYS_KeyPressed(SYS_KEY_LEFT))
			g_entities[MAINSHIP_ENTITY].vel.x = -HORIZONTAL_SHIP_VEL;
		else if (SYS_KeyPressed(SYS_KEY_RIGHT))
			g_entities[MAINSHIP_ENTITY].vel.x = +HORIZONTAL_SHIP_VEL;
		else
			g_entities[MAINSHIP_ENTITY].vel.x = 0.f;
	}

	//-----------------------------------------------------------------------------
	// Game state (apart from entities & other stand-alone modules)
	float g_time = 0.f;

	//-----------------------------------------------------------------------------
	// Main
	int Main(void)
	{
		// Start things up & load resources ---------------------------------------------------
		StartGame();

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
			g_time += 1.f / 60.f;
		}
		EndGame();

		return 0;
	}
}
