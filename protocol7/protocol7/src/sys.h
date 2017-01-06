/*
 * sys.h - Declaration of system-dependent functions
 */
#pragma once
#ifndef	P7_SYS_H_
#define P7_BASE_H_

#pragma region Screen
// ============================================================================
//	Resolution (Real + Virtual)

//	Portrait 2:3 aspect ratio
enum
{
	SYS_WIDTH = 512,
	SYS_HEIGHT = 768
};
const bool	 SYS_FULLSCREEN = false;

//	Game window co-ordinate system: width = 1000 px, height = 1500 px
//	Origin is bottom left corner of the window.
enum
{
	G_WIDTH = 1000,
	G_HEIGHT = 1500
};
#pragma endregion

#pragma region Platform
// ============================================================================
//	Platform layer
void	SYS_Pump();
void	SYS_Show();
bool	SYS_GottaQuit();
void	SYS_Sleep(int ms);
bool	SYS_KeyPressed(int key);
ivec2	SYS_MousePos();
bool	SYS_MouseButtonPressed(int button);
#pragma endregion

#pragma region Key Bindings
//-----------------------------------------------------------------------------
#ifdef _WINDOWS

#define SYS_KEY_UP    VK_UP
#define SYS_KEY_DOWN  VK_DOWN
#define SYS_KEY_LEFT  VK_LEFT
#define SYS_KEY_RIGHT VK_RIGHT

#define SYS_MB_LEFT   VK_LBUTTON
#define SYS_MB_RIGHT  VK_RBUTTON
#define SYS_MB_MIDDLE VK_MBUTTON

//-----------------------------------------------------------------------------
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#if defined(__MACH__) && TARGET_OS_MAC && !TARGET_OS_IPHONE

#define SYS_KEY_UP    GLFW_KEY_UP
#define SYS_KEY_DOWN  GLFW_KEY_DOWN
#define SYS_KEY_LEFT  GLFW_KEY_LEFT
#define SYS_KEY_RIGHT GLFW_KEY_RIGHT

#define SYS_MB_LEFT   GLFW_MOUSE_BUTTON_LEFT
#define SYS_MB_RIGHT  GLFW_MOUSE_BUTTON_RIGHT
#define SYS_MB_MIDDLE GLFW_MOUSE_BUTTON_MIDDLE

#endif //defined(__MACH__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
#endif //defined(__APPLE__)
//-----------------------------------------------------------------------------

#pragma endregion

#endif // !P7_SYS_H_