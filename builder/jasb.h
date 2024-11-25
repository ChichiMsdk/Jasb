/******************************************************************************
 * 						JASB. Just A "Simple" Builder.                        *
 ******************************************************************************
 * JASB will look for all the ".o.json" files and put them in one             *
 * compile_commands.json at the root directory. It is intended to be used with*
 * a Makefile that will compile it and then use after each build. As sed could*
 * differ from os and shell's this one should actually be portable.           *
 ******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
/* 
 * NOTE: Let's try this since I just need portable code
 */
#include <threads.h>
#include "TracyC.h"

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define YMB [[maybe_unused]]

#ifdef _WIN32

#pragma comment(lib, "shlwapi.lib")

	#define TRACY_PATH "C:\\Lib\\tracy-0.11.1\\public"
	#define TRACYTRACY_PATH "C:\\Lib\\tracy-0.11.1\\public\\tracy"

	#define GLFW_PATH "C:\\Lib\\glfw\\include" 
	#define GLFWLIB_PATH "C:\\Lib\\glfw\\lib-vc2022"
	#define GLFWLIB "gflw3_mt"

	#define VULKAN_PATH "C:/vulkan/Include"
	#define VULKANLIB_PATH "C:/vulkan/Lib"
	#define VULKANLIB "vulkan-1"
	#define OPENGLLIB "opengl32"

#elif __linux__
	#include <stdarg.h>

	#define TRACY_PATH "/home/chichi/tracy/public"
	#define TRACYTRACY_PATH "/home/chichi/tracy/public/tracy"

	#define GLFW_PATH ""
	#define GLFWLIB_PATH ""
	#define GLFWLIB "glfw"

	#define VULKAN_PATH ""
	#define VULKANLIB_PATH ""
	#define VULKANLIB "vulkan"
	#define OPENGLLIB "GL"

#endif // WIN32


/*****************************************************************************/
/*                              Macro stuff                                  */
/*****************************************************************************/

void MakeCleanImpl(void *none, ...);
#define MakeClean(...) MakeCleanImpl(__VA_ARGS__, NULL)
