/*
* game5.cpp - Added more gameplay features:
*				- Algorithmic rock generation
*/
#include "stdafx.h"
#include "base.h"
#include "sys.h"
#include "core.h"

namespace Game5
{
	//=============================================================================
	// Logger
	inline void LOG(char *msg) { printf(msg); } // Comment to disable logging

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
	static const float SHIP_W = 250;
	static const float SHIP_H = 270;

	// Ship movement
	static const float SHIP_CRUISE_SPEED = 25.f;
	static const float SHIP_START_SPEED = 5.f;
	static const float SHIP_INC_SPEED = .5f;
	static const float HORIZONTAL_SHIP_VEL = 10.f;
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
	static const float JUICE_CHANCE_PER_PIXEL = 1.f / 1000.f;
	static const float GEN_IN_ADVANCE = 400.f;

	// Game state
	static const float RACE_END = 100000.f;
	static const float FPS = 60.f;
	static const float FRAMETIME = (1.f / FPS);

	static const float STARTING_TIME = 2.f;
	static const float DYING_TIME = 2.f;
	static const float VICTORY_TIME = 8.f;

	static const float FIRST_CHALLENGE = 3000.f;

	// Level generation
	static const float PATH_TWIST_RATIO = 0.5f; // This means about 30 degrees maximum
	static const float PATH_WIDTH = (2.f * MAINSHIP_RADIUS);
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
	// Textures
	int g_ship_LL, g_ship_L, g_ship_C, g_ship_R, g_ship_RR;
	int g_bkg, g_pearl, g_energy, g_fuel, g_star, g_juice, g_mine, g_drone;
	int g_rock[5];

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
		int		texture;
		bool	tex_additive;
		bool	has_shadow;
		rgba	color;
	};
	Entity   g_entities[MAX_ENTITIES];
	static Entity *MAIN_SHIP = &g_entities[MAINSHIP_ENTITY];

	//-----------------------------------------------------------------------------
	void InsertEntity(EType type, vec2 pos, vec2 vel, float radius, int tex,
		bool has_shadow, bool additive = false)
	{
		for (int i = 0; i < MAX_ENTITIES; i++)
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
				g_entities[i].color = MakeRGBA(1.f, 1.f, 1.f, 1.f);
				break;
			}
		}
	}

	//-----------------------------------------------------------------------------
	void LoadResources()
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
		g_pearl = CORE_LoadBmp("data/Pearl.bmp", false);
		g_energy = CORE_LoadBmp("data/Energy.bmp", false);
		g_fuel = CORE_LoadBmp("data/Fuel.bmp", false);
		g_star = CORE_LoadBmp("data/Star.bmp", false);
		g_juice = CORE_LoadBmp("data/Juice.bmp", false);
		g_mine = CORE_LoadBmp("data/Mine.bmp", false);
		g_drone = CORE_LoadBmp("data/Drone0.bmp", false);
	}

	//-----------------------------------------------------------------------------
	void UnloadResources()
	{
		// Release resources
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
		CORE_UnloadBmp(g_pearl);
		CORE_UnloadBmp(g_energy);
		CORE_UnloadBmp(g_fuel);
		CORE_UnloadBmp(g_star);
		CORE_UnloadBmp(g_juice);
		CORE_UnloadBmp(g_mine);
		CORE_UnloadBmp(g_drone);
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

				// Draw shadow first if valid
				if (g_entities[i].has_shadow)
					CORE_RenderCenteredSprite(vadd(vsub(pos, vmake(0.f, g_camera_offset)),
						vmake(0.f, -SHADOW_OFFSET)), vmake(size.x * SPRITE_SCALE * g_entities[i].tex_scale
						* SHADOW_SCALE, size.y * SPRITE_SCALE * g_entities[i].tex_scale * SHADOW_SCALE),
						g_entities[i].texture, MakeRGBA(0.f, 0.f, 0.f, 0.4f), g_entities[i].tex_additive);

				// Draw actual entity
				CORE_RenderCenteredSprite(vsub(pos, vmake(0.f, g_camera_offset)),
					vmake(size.x * SPRITE_SCALE * g_entities[i].tex_scale, size.y * SPRITE_SCALE * g_entities[i].tex_scale), g_entities[i].texture, g_entities[i].color, g_entities[i].tex_additive);
			}
		}

		// Draw the UI
		if (g_gs != GS_VICTORY)
		{
			// Energy bar
			float energy_ratio = MAIN_SHIP->energy / MAX_ENERGY;
			CORE_RenderCenteredSprite(
				vmake(ENERGY_BAR_W / 2.f, energy_ratio * ENERGY_BAR_H / 2.f),
				vmake(ENERGY_BAR_W, ENERGY_BAR_H * energy_ratio),
				g_energy, COLOR_WHITE, true);

			// Fuel bar
			float fuel_ratio = MAIN_SHIP->fuel / MAX_FUEL;
			CORE_RenderCenteredSprite(
				vmake(G_WIDTH - FUEL_BAR_W / 2.f, fuel_ratio * FUEL_BAR_H / 2.f),
				vmake(FUEL_BAR_W, FUEL_BAR_H * fuel_ratio),
				g_fuel, COLOR_WHITE, true);

			// Progress bar
			int num_chunks = (int)((g_current_race_pos / RACE_END) * MAX_CHUNKS);
			for (int i = 0; i < num_chunks; i++)
				CORE_RenderCenteredSprite(
					vmake(G_WIDTH - 100.f, 50.f + i * 50.f),
					vmake(CHUNK_W, CHUNK_H),
					g_pearl);
		}
	}

	//-----------------------------------------------------------------------------
	void ResetNewGame()
	{
		// Reset everything for a new game...
		g_next_challenge_area = FIRST_CHALLENGE;
		g_last_conditioned = vmake(.5f * G_WIDTH, 0.f);
		g_current_race_pos = 0.f;
		g_camera_offset = 0.f;
		g_rock_chance = START_ROCK_CHANCE_PER_PIXEL;
		g_gs = GS_START;
		g_gs_timer = 0.f;

		// Start logic
		for (size_t i = 0; i < MAX_ENTITIES; i++)
			g_entities[i].type = E_NULL;

		// Initialize main ship
		InsertEntity(E_MAIN, vmake(G_WIDTH / 2.0, G_HEIGHT / 8.f), vmake(0.f, SHIP_START_SPEED), MAINSHIP_RADIUS, g_ship_C, true);
	}

	//-----------------------------------------------------------------------------
	// Generate the level obstacles
	void GenNextElements()
	{
		// Called every game loop, but only does work when we are close to the next "challenge area"
		if (g_current_race_pos + G_HEIGHT > g_next_challenge_area)
		{
			float current_y = g_next_challenge_area;
			LOG("\"Current: %f\n\", g_next_challenge_area");

			// Choose how many layers of rocks
			size_t nlayers = (int)CORE_URand(1, 20);
			LOG("\" nlayers: %d\n\", nlayers");
			for (size_t i = 0; i < nlayers; i++)
			{
				LOG("\"  where: %f\n\", current_y");

				// Choose pass point
				float displace = (current_y - g_last_conditioned.y) * PATH_TWIST_RATIO;
				float bracket_left = g_last_conditioned.x - displace;
				float bracket_right = g_last_conditioned.x + displace;
				bracket_left = Max(bracket_left, PATH_WIDTH);
				bracket_right = Max(bracket_right, G_WIDTH - PATH_WIDTH);
				g_last_conditioned.y = current_y;
				g_last_conditioned.x = CORE_FRand(bracket_left, bracket_right);

				/*InsertEntity(E_JUICE,
				vmake(g_last_conditioned.x, g_last_conditioned.y),
				vmake(0.f,0.f),
				JUICE_RADIUS, g_juice, false, true);*/

				// Choose how many rocks
				size_t nrocks = (int)CORE_URand(1, 2);
				LOG("\"  nrocks: %d\n\", nrocks");

				// Gen rocks
				for (size_t i = 0; i < nrocks; i++)
				{
					// Find a valid pos
					vec2 rock_pos;
					while (true)
					{
						rock_pos = vmake(CORE_FRand(0.f, G_WIDTH), current_y);
						if (rock_pos.x + ROCK_RADIUS < g_last_conditioned.x - PATH_WIDTH ||
							rock_pos.x - ROCK_RADIUS > g_last_conditioned.x + PATH_WIDTH)
							break;
					}

					// Insert obstacle
					EType t = E_ROCK;
					int tex = g_rock[1/*CORE_URand(0, 4)*/];
					if (CORE_RandChance(0.1f)) { t = E_MINE;  tex = g_mine; }
					else if (CORE_RandChance(0.1f)) { t = E_DRONE; tex = g_drone; }

					InsertEntity(t, rock_pos, vmake(CORE_FRand(-.5f, +.5f), CORE_FRand(-.5f, +.5f)),
						ROCK_RADIUS, tex, true);
				}

				current_y += CORE_FRand(300.f, 600.f);
			}

			g_next_challenge_area = current_y + CORE_FRand(.5f * G_HEIGHT, 1.5f * G_HEIGHT);
			LOG("\"Next: %f\n\n\", g_next_challenge_area");
		}
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
				LOG("\"- energy: %f, fuel: %f\n\", MAIN_SHIP.energy, MAIN_SHIP.fuel");
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
					g_entities[i].type = E_NULL;
			}

			// Advance 'stars'
			if (g_entities[i].type == E_STAR)
			{
				g_entities[i].tex_scale *= 1.008f;
			}
		}

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
								MAIN_SHIP->energy = SafeSub(MAIN_SHIP->energy, ROCK_CRASH_ENERGY_LOSS);
								MAIN_SHIP->vel.y = SHIP_START_SPEED;
								g_entities[i].vel = vscale(vunit(vsub(g_entities[i].pos, MAIN_SHIP->pos)), CRASH_VEL);
								g_entities[i].energy = 0;
							}
							break;

						case E_JUICE:
							MAIN_SHIP->fuel = SafeAdd(MAIN_SHIP->fuel, JUICE_FUEL, MAX_FUEL);
							g_entities[i].type = E_NULL;
							break;

						case E_MINE:
							MAIN_SHIP->energy = SafeSub(MAIN_SHIP->energy, MINE_CRASH_ENERGY_LOSS);
							MAIN_SHIP->vel.y = SHIP_START_SPEED;
							g_entities[i].type = E_NULL;
							break;

						case E_DRONE:
							MAIN_SHIP->energy = SafeSub(MAIN_SHIP->energy, MINE_CRASH_ENERGY_LOSS);
							MAIN_SHIP->vel.y = SHIP_START_SPEED;
							g_entities[i].type = E_NULL;
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
								g_entities[i].type = E_NULL;
								g_entities[j].type = E_NULL;

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
					ROCK_RADIUS, g_juice, false, true);
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
				ResetNewGame();
			}
			break;

		case GS_PLAYING:
			if (MAIN_SHIP->energy <= 0.f || MAIN_SHIP->fuel <= 0.f)
			{
				g_gs = GS_DYING;
				g_gs_timer = 0.f;
				MAIN_SHIP->texture = g_ship_RR;
			}
			break;

		case GS_VICTORY:
			if (CORE_RandChance(1.f / 10.f))
				InsertEntity(E_STAR,
					MAIN_SHIP->pos,
					vadd(MAIN_SHIP->vel, vmake(CORE_FRand(-5.f, 5.f), CORE_FRand(-5.f, 5.f))),
					0, g_star, false, true);
			if (g_gs_timer >= VICTORY_TIME)
				ResetNewGame();
			break;
		}

		g_time_from_last_rocket += FRAMETIME;
	}

	//-----------------------------------------------------------------------------
	void ProcessInput()
	{
		bool left = SYS_KeyPressed(SYS_KEY_LEFT);
		bool right = SYS_KeyPressed(SYS_KEY_RIGHT);

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

		float tilt = MAIN_SHIP->tilt;
		if (tilt < -.6f * SHIP_MAX_TILT)	  MAIN_SHIP->texture = g_ship_LL;
		else if (tilt < -.2f * SHIP_MAX_TILT) MAIN_SHIP->texture = g_ship_L;
		else if (tilt < +.2f * SHIP_MAX_TILT) MAIN_SHIP->texture = g_ship_C;
		else if (tilt < +.6f * SHIP_MAX_TILT) MAIN_SHIP->texture = g_ship_R;
		else                                  MAIN_SHIP->texture = g_ship_RR;
	}

	//-----------------------------------------------------------------------------
	// Game state (apart from entities & other stand-alone modules)
	float g_time = 0.f;

	//-----------------------------------------------------------------------------
	// Main
	int Main(void)
	{
		// Start things up & load resources ---------------------------------------------------
		LoadResources();
		ResetNewGame();

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
		UnloadResources();

		return 0;
	}
}
