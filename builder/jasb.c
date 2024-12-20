/******************************************************************************
 |                      JASB. Just A "Simple" Builder.                        |
 ******************************************************************************
 | JASB will look for all the ".o.json" files and put them in one             |
 | compile_commands.json at the root directory. It is intended to be used with|
 | a Makefile that will compile it and then use after each build. As sed could|
 | differ from os and shell's this one should actually be portable.           |
 ******************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include "jasb.h"
#include "jasb_execute.c"
#include "jasb_utils.c"
#include "jasb_strings.c"
#include "jasb_errors.c"
#include "jasb_execute.c"
#include "jasb_threads.c"

#include "TracyC.h"

MemChef gChef = {0};

#pragma comment(lib, "shlwapi.lib")

bool gbClean = false;
bool gbDebug = true;
bool gbRelease = false;

bool gbTracy = false;
bool gbAsan = false;
bool gbTest = false;

bool gbVulkan = true;
bool gbOpenGL = true;
bool gbD3D11 = false;
bool gbD3D12 = false;

#ifdef __linux__
bool gbGLFW3 = true;
#elif _WIN32
bool gbGLFW3 = false;
#endif

Command
CommandInit(void)
{	
	Command cmd = {
		.pRootFolder = STR("src"),
		.pOutputName = STR("yuseong"),
		.pExtension = STR(EXTENSION),
		.pBuildDir = STR("build"),
		.pObjDir = STR("build" SLASH "obj"),
		.pCc = STR("clang-cl"),
		.pCpp = STR("clang-cl"),
		.pLinker = STR("link"),
		.pGlslc = STR("glslc"),
		.pSrcDir = STR("src"),
		.pShaderDir = STR("src" SLASH "shaders"),
		.pShaderObjsDir = STR("build" SLASH "obj" SLASH "shaders"),
		.pCflags = STR("/FS"),
		.pLibPath = STR(""),
		.pIncludeDirs = STR(""),
		.pLibs = STR(""),
		.pDefines = STR(""),
	};

	char *INCLUDEFLAG = NULL; char *LIBFLAG = NULL; char *LIBDEPEND = NULL;
	char *OPTIFLAGS = NULL; char *DEBUGMODEFLAGS = NULL; char *DEFINEFLAG = NULL;


	if ((StrIsEqual(cmd.pCc, "clang") || (StrIsEqual(cmd.pCc, "gcc"))))
	{
		OPTIFLAGS = STR("-O3");
		DEBUGMODEFLAGS = STR("-O0");
		DEFINEFLAG = STR("-D");
		INCLUDEFLAG = STR("-I");
		LIBFLAG = STR("-L");
		LIBDEPEND = STR("-l");
		SELF_APPEND(cmd.pCflags, " -Wall -Wextra");
		SELF_APPEND(cmd.pCflags, " -g3");
		SELF_APPEND(cmd.pCflags, " -fno-inline", " -fno-omit-frame-pointer");
		SELF_APPEND(cmd.pCflags, " -Wno-missing-field-initializers", " -Wno-unused-but-set-variable");
		SELF_APPEND(cmd.pCflags, " -Wno-uninitialized");
		SELF_APPEND(cmd.pCflags, " -Wvarargs");
	}
	/* NOTE: only expects MSVC */
#ifdef _WIN32 
	else
	{
		OPTIFLAGS = STR("/O2");
		DEBUGMODEFLAGS = STR("/Od");
		DEFINEFLAG = STR("/D");
		INCLUDEFLAG = STR("/I");
		LIBFLAG = STR("/LIBPATH:");
		LIBDEPEND = STR(".lib");
		SELF_APPEND(cmd.pCflags, " /Wall");
		if (StrIsEqual("cl", cmd.pCc))
			SELF_APPEND(cmd.pCflags, " /std:clatest");
		if (StrIsEqual("clang-cl", cmd.pCc))
		{
			SELF_APPEND(cmd.pCflags, " -Wno-unsafe-buffer-usage -Wno-static-in-inline -Wno-switch-enum -Wno-float-equal");
			SELF_APPEND(cmd.pCflags, " -Wno-pre-c11-compat -Wno-c23-extensions -Wno-documentation -Wno-documentation-pedantic");
			SELF_APPEND(cmd.pCflags, " -Wno-declaration-after-statement -Wno-extra-semi-stmt");
			SELF_APPEND(cmd.pCflags, " -Wno-gnu-zero-variadic-macro-arguments -Wno-language-extension-token");
			SELF_APPEND(cmd.pCflags, " -Wno-double-promotion -Wno-shorten-64-to-32 -Wno-implicit-int-conversion");
			SELF_APPEND(cmd.pCflags, " -Wno-implicit-int-float-conversion -Wno-reserved-identifier");
			SELF_APPEND(cmd.pCflags, " -Wno-reserved-macro-identifier -Wno-sign-conversion");
		}

		SELF_APPEND(cmd.pCflags, " /nologo");
		SELF_APPEND(cmd.pCflags, " /Zi");
	}
	SELF_PREPEND_WITH_FLAGS(cmd.pDefines, DEFINEFLAG, "PLATFORM_WINDOWS");
	ADD_FLAGS(cmd.pLibs, LIBDEPEND, "shlwapi", "shell32", "gdi32", "winmm", "user32");

#elif __linux__
	cmd.pDEFINES = STR("-DPLATFORM_LINUX -DYGLFW3 ");
	SELF_APPEND(cmd.pCFLAGS, "-ggdb3");
	SELF_APPEND(cmd.pLIBS, "-lwayland-client -lxkbcommon -lm ");
#endif
	SELF_PREPEND_WITH_FLAGS(cmd.pIncludeDirs, INCLUDEFLAG, "src", "src/core", "thirdparty");
	cmd.pCppFlags = STR("");

	if (gbDebug)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDefines, DEFINEFLAG, "_DEBUG", "DEBUG", "YUDEBUG ");
		SELF_APPEND(cmd.pCflags, " ", DEBUGMODEFLAGS);
	}
	else if (gbRelease)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDefines, DEFINEFLAG, "_RELEASE", "RELEASE", "YURELEASE ");
		SELF_APPEND(cmd.pCflags, " ", OPTIFLAGS);
	}

	if (gbTest) SELF_PREPEND_WITH_FLAGS(cmd.pDefines, DEFINEFLAG, "TESTING ");

	if (gbVulkan)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pLibPath, LIBFLAG, VULKANLIB_PATH);
		SELF_PREPEND_WITH_FLAGS(cmd.pIncludeDirs, INCLUDEFLAG, VULKAN_PATH);
		ADD_FLAGS(cmd.pLibs, LIBDEPEND, VULKANLIB);
	}
	if (gbOpenGL)
	{
		ADD_FLAGS(cmd.pLibs, LIBDEPEND, OPENGLLIB);
	}
#ifdef _WIN32
	else if (gbD3D11) { ADD_FLAGS(cmd.pLibs, LIBDEPEND, "d3dcompiler", "d3d11", "dxgi", "dxguid"); }
	else if (gbD3D12) { }
#endif
	if (gbGLFW3)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pIncludeDirs, INCLUDEFLAG, GLFW_PATH);
		SELF_PREPEND_WITH_FLAGS(cmd.pLibPath, LIBFLAG, GLFWLIB_PATH);
		ADD_FLAGS(cmd.pLibs, LIBDEPEND, GLFWLIB);
	}
	if (gbTracy)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDefines, DEFINEFLAG, "TRACY_ENABLE");
		/* CPPFLAGS =-Wno-format */
		/* Also, compile cpp flags with different struct ? */
	}
	if (gbAsan)
	{
		SELF_APPEND(cmd.pCflags, " ", "-fsanitize=address");
	}
	SELF_PREPEND_WITH_FLAGS(cmd.pIncludeDirs, INCLUDEFLAG, TRACY_PATH, TRACYTRACY_PATH);
	return cmd;
}

bool
ArgsCheck(int argc, char** ppArgv)
{
	if (argc < 2)
		return true;
	if (StrIsEqual(ppArgv[1], "-h"))
	{
		fprintf(stderr, "Usage: jasb\n");
		return false;
	}
	int i = -1;
	while (++i < argc)
	{
		if (StrIsEqual(ppArgv[i], "vk") || StrIsEqual(ppArgv[i], "vulkan") || StrIsEqual(ppArgv[i], "VULKAN")
				|| StrIsEqual(ppArgv[i], "Vulkan"))
		{
			gbVulkan = true;
			fprintf(stderr, "Vulkan backend chosen\n");
			continue;
		}
		if (StrIsEqual(ppArgv[i], "TRACY") || StrIsEqual(ppArgv[i], "tracy"))
		{
			gbTracy = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "gl") || StrIsEqual(ppArgv[i], "opengl") || StrIsEqual(ppArgv[i], "OpenGL"))
		{
			gbOpenGL = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "D3D11"))
		{
			gbD3D11 = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "D3D12"))
		{
			gbD3D12 = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "GLFW3"))
		{
			gbGLFW3 = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "TEST"))
		{
			gbTest = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "DEBUG"))
		{
			gbDebug = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "RELEASE"))
		{
			gbRelease = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "ASAN"))
		{
			gbAsan = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "clean"))
			gbClean = true;
	}
	return true;
}

yError
JoinThreads(threadStruct* pArgs, thrd_t* pThreads, size_t elemCount)
{
	TracyCZoneN(ctx, "Join threads", 1);
	int		threadResult;
	int		res				= 0;
	size_t	completedCount	= 0;

	while(completedCount < elemCount)
	{
		for (size_t i = 0; i < elemCount; i++)
		{
			if (!pArgs[i].finished) continue;

			TracyCZoneNC(ctx2, "Joining 1 thread", 0xFF0000, 1);
			res = 0;
			threadResult = thrd_join(pThreads[i], &res);
			if (threadResult != thrd_success || threadResult != 0)
			{
				TracyCZoneEnd(ctx2);
				goto ReturnError;
			}

			pArgs[i].finished = false;
			completedCount++;
			TracyCZoneEnd(ctx2);
		}
	}
	TracyCZoneEnd(ctx);
	/* printf("\n"); */
	return Y_SUCCESS;

ReturnError:
	TracyCZoneEnd(ctx);
	return Y_ERROR_JOIN_THREAD;
}

/* FIXME: This is actual garbage fix this shit and make better error handling */
yError
CompileCfiles(Command* pCmd, FileList* pList, char **ppOut, bool silent, bool bMultiThread, bool debug)
{
	yError result = Y_SUCCESS;

	char*	OUTFLAG				= NULL;
	char*	MODEFLAG			= NULL;
	char*	DEBUG_FOLDER		= NULL;
	char*	OUT_DEBUG_FOLDER	= NULL;

	if (StrIsEqual("clang", pCmd->pCc) || StrIsEqual("gcc", pCmd->pCc))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}
	else
	{
		OUTFLAG = STR("/Fo:");
		MODEFLAG = STR("/c /Tc");
		DEBUG_FOLDER = STR("/Fd");
	}

	// NOTE: Making the path .o files for linker
	size_t	count				= 0;
	size_t	total				= 1000000;
	size_t	track				= 0;
	Command	a					= *pCmd;

	char**			ppJson		= malloc(sizeof(char *) * pList->elemCount);
	thrd_t*			pThreads	= malloc(sizeof(thrd_t) * pList->elemCount);
	threadStruct*	pArgs		= malloc(sizeof(threadStruct) * pList->elemCount);
	char*			pTemp		= malloc(sizeof(char) * total + (pList->elemCount) + 1);
	mtx_t*			pMtx		= malloc(sizeof(mtx_t));

	MutexInit(pMtx, mtx_plain);

	for (size_t i = 0; i < pList->elemCount; i++)
	{
		track += strlen(pList->pFiles[i].pObjName);
		if (track >= total)
		{ 
			fprintf(stderr, "Too long linker command %zu\n", track);
			exit(1);
		}

		char*	pOutputName = STR(pList->pFiles[i].pObjName);
		char*	pFilePath	= STR(pList->pFiles[i].pFullPath);
		char*	pRawName	= STR(pList->pFiles[i].pFullPath);

		size_t rawSize = strlen(pRawName);
		for (size_t iterator = 0; iterator < rawSize; iterator++)
		{
			if (pRawName[iterator] == '.')
			{
				pRawName[iterator] = 0;
				break;
			}
		}

		if (StrIsEqual("cl", pCmd->pCc) || StrIsEqual("clang-cl", pCmd->pCc))
		{
			OUT_DEBUG_FOLDER = STR(DEBUG_FOLDER);
			SELF_APPEND(OUT_DEBUG_FOLDER, pCmd->pBuildDir, SLASH, pRawName,".pdb");
		}

		if (StrIsEqual("cl", pCmd->pCc) || StrIsEqual("clang-cl", pCmd->pCc))
			SELF_APPEND(pOutputName, "bj");

		count += sprintf(pTemp + count, "%s ", pOutputName);
		if (StrIsEqual("cl", pCmd->pCc) || StrIsEqual("clang-cl", pCmd->pCc))
			pOutputName = PUTINQUOTE(pOutputName);

		ppJson[i] = STR("");
		char*	pPchFlag = "";
		char*	pTimeReportFlag = "";

			/*
			 * if (StrIsEqual(pFilePath, "src"SLASH"renderer"SLASH"vulkan"SLASH"vulkan_loader.c"))
			 * {
			 * 	pPchFlag = "-include-pch src"SLASH"pch"SLASH"stb_image_pch.h.pch"
			 * 		" -include-pch src"SLASH"pch"SLASH"windows_pch.h.pch";
			 * 
			 * 	pTimeReportFlag = "";
			 * }
			 * else
			 * {
			 * 	pTimeReportFlag = "";
			 * 	pPchFlag = "-include-pch src"SLASH"pch"SLASH"windows_pch.h.pch";
			 * 	pPchFlag = "";
			 * }
			 */

		SELF_APPEND_SPACE(ppJson[i], a.pCc, pTimeReportFlag, a.pCflags, a.pDefines, a.pIncludeDirs, pPchFlag);
		SELF_APPEND_SPACE(ppJson[i], MODEFLAG, pFilePath, OUTFLAG, pOutputName, OUT_DEBUG_FOLDER);
		/* SELF_APPEND_SPACE(ppJson[i], a.pCC, a.pCFLAGS, a.pDEFINES, a.pINCLUDE_DIRS, MODEFLAG, fname, OUTFLAG, outname); */
		bool bExec = false;
		if (IsOutdated(pFilePath, pOutputName))
		{
			bExec = true;
		}

		if (bMultiThread)
		{
			pArgs[i].pThreadName = pFilePath;
			pArgs[i].pDependencyPath = pFilePath;
			pArgs[i].pTargetPath = pOutputName;
			pArgs[i].silent = silent;
			pArgs[i].debug	= debug;
			pArgs[i].id = i;
			pArgs[i].pMutex = pMtx;
			pArgs[i].finished = false;
			pArgs[i].pCmd = ppJson[i];
			pArgs[i].noExec = bExec;
			thrd_create(&pThreads[i], ThreadExec, &pArgs[i]);
		}
		else
		{
			result = EXECUTE(ppJson[i], pFilePath, pOutputName, silent, debug);
			if (result != 0)
			{
				MultiFree(pThreads, ppJson, pArgs);
				return Y_ERROR_BUILD;
			}
		}
	}
	/*
	 * TODO: Idea -> Queue of ".o" files to wait to exist before linking
	 */
	/* CompileCmdJson(ppJson, pList->nbElems); */
	if (bMultiThread)
		result = JoinThreads(pArgs, pThreads, pList->elemCount);
	MultiFree(pThreads, ppJson, pArgs);

	if (result != Y_SUCCESS)
	{
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, GetErrorMsg(result));
		exit(1);
	}

	if (count > 0)
		count--;
	pTemp[count] = 0;
	if (!pTemp)
	{
		free(pTemp);
		result = Y_ERROR_EMPTY;
	}
	*ppOut = pTemp;
	return result;
}

yError
Link(Command* pCmd, char *pObj, const char *pOutName, bool silent, bool debug)
{
	char *pCommand = STR("");
	Command a = *pCmd;

	char*	OUTFLAG		= NULL;
	char*	MODEFLAG	= NULL;
	char*	LLD_FLAGS	= NULL;
	bool	bRebuild	= false;

	char* pTemp = STR(pObj);
	char* pToken = strtok(pTemp, " ");
	while (pToken != NULL)
	{
		if (IsOutdated(pToken, pOutName))
			bRebuild = true;
		pToken = strtok(NULL, " ");
	}
	if (bRebuild == false)
	{
		printf("Nothing to be done.\n");
		return Y_SUCCESS;
	}

	if (StrIsEqual("clang", pCmd->pCc) || StrIsEqual("gcc", pCmd->pCc))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
		LLD_FLAGS = STR("-fuse-ld=lld");
	}
	else
	{
		OUTFLAG = STR("/OUT:");
		SELF_APPEND(a.pLinker, " /NOLOGO");
	}
	SELF_APPEND(OUTFLAG, pOutName);

	/* SELF_APPEND_SPACE(pCommand, a.pCC, a.pCFLAGS, a.pDEFINES, pObj, OUTFLAG, pOutName, pCmd->pLIB_PATH, pCmd->pLIBS); */
	SELF_APPEND_SPACE(pCommand, a.pLinker, LLD_FLAGS, a.pCflags, a.pDefines);
	SELF_APPEND_SPACE(pCommand, pObj, OUTFLAG, pCmd->pLibPath, pCmd->pLibs);

	TracyCZoneNC(linkingpart, "Link", 0x00001, 1);
	if (EXECUTE(pCommand, pOutName, pOutName, silent, debug))
	{
		return Y_ERROR_EXEC;
	}
	TracyCZoneEnd(linkingpart);
	return Y_SUCCESS;
}

typedef struct shaderStruct
{
	Command*	pCmd;
	FileList*	pList;
	bool		silent;
} shaderStruct;

int
CompileShaders(void* args)
{
	TracyCSetThreadName("compile shaders");
	shaderStruct* pTemp = (shaderStruct*)(args);
	TracyCZoneNC(threading, __FUNCTION__, 0x8800ff, 1);

	FileList* pList = pTemp->pList;
	Command* pCmd = pTemp->pCmd;
	bool silent = pTemp->silent;

	yError result = Y_SUCCESS;
	if (pList->elemCount <= 0)
	{ fprintf(stderr, "No files to compile found ..\n"); result = Y_ERROR_EMPTY; return result; }

	size_t			total		= 1000000;
	size_t			track		= 0;

	thrd_t*			pThreads	= malloc(sizeof(thrd_t) * pList->elemCount);
	threadStruct*	pArgs		= malloc(sizeof(threadStruct) * pList->elemCount);

	Command a = *pCmd;

	char*	OUTFLAG = STR("-o");

	for (int i = 0; i < pList->elemCount; i++)
	{
		char *pCommand = STR("");
		SELF_APPEND_SPACE(pCommand, a.pGlslc, pList->pFiles[i].pFullPath, OUTFLAG, pList->pFiles[i].pObjName);
		bool debug = false;

		pArgs[i].pThreadName = pList->pFiles[i].pFullPath;
		pArgs[i].pDependencyPath = pList->pFiles[i].pFullPath;
		pArgs[i].pTargetPath = pList->pFiles[i].pObjName;
		pArgs[i].silent = silent;
		pArgs[i].debug = debug;
		pArgs[i].id = i;
		pArgs[i].finished = false;
		pArgs[i].pCmd = pCommand;

		thrd_create(&pThreads[i], ThreadExecShaders, &pArgs[i]);
	}

	TracyCZoneN(ctx, "Join threads", 1);
	size_t	completedCount = 0;
	while(completedCount < pList->elemCount)
	{
		for (size_t i = 0; i < pList->elemCount; i++)
		{
			if (!pArgs[i].finished) continue;
			TracyCZoneNC(ctx2, "Joining 1 thread", 0xFF0000, 1);
			int res = 0;
			int	threadRes	= thrd_join(pThreads[i], &res);
			if ( threadRes != thrd_success || threadRes != 0)
			{
				free(pThreads);
				free(pArgs);
				TracyCZoneEnd(ctx);
				TracyCZoneEnd(ctx2);
				exit(1);

				return Y_ERROR_BUILD;
			}
			pArgs[i].finished = false;
			completedCount++;
			TracyCZoneEnd(ctx2);
		}
	}
	TracyCZoneEnd(ctx);
	/* printf("\n"); */

	free(pThreads);
	free(pArgs);

	TracyCZoneEnd(threading);
	thrd_exit(result);
	return result;
}

yError
ShadersBuild(Command* pCmd, FileList* pShadersList, bool silent)
{
	/* NOTE: I guess it's better than stack since the main thread shouldn't wait */
	shaderStruct*	pShaderThreadStruct	= malloc(sizeof(shaderStruct));
	thrd_t*			pThread				= malloc(sizeof(thrd_t));
	
	pShaderThreadStruct->pCmd			= pCmd;
	pShaderThreadStruct->pList			= pShadersList;
	pShaderThreadStruct->silent			= silent;

	if (pShadersList->elemCount <= 0) 
	{ 
		fprintf(stderr, "No files to compile shaders found ..\n"); 
		MultiFree(pThread, pShaderThreadStruct);
		return Y_ERROR_EMPTY; 
	}
	else
		thrd_create(pThread, CompileShaders, pShaderThreadStruct);

	/* FIXME: pShaderThreadStruct AND pThread are lost pointers */ 
	return Y_SUCCESS;
}

/* NOTE: Threads -> I don't free anything */
yError
Build(Command* pCmd, FileList* pCfilesList, FileList* pShadersList, bool bMultiThread, bool debug)
{
	char *pOutObjs = NULL;
	char *pOutput = STR("");
	SELF_APPEND(pOutput, pCmd->pBuildDir, SLASH, pCmd->pOutputName, pCmd->pExtension);
	bool silent	= false;

	if (pShadersList->elemCount > 0)
		ShadersBuild(pCmd, pShadersList, true);

	if (pCfilesList->elemCount <= 0) 
	{ 
		fprintf(stderr, "No files to compile found ..\n"); 
		return Y_ERROR_EMPTY; 
	}
	else
	{
		JASB_CHECK(CompileCfiles(pCmd, pCfilesList, &pOutObjs, silent, bMultiThread, debug));
		JASB_CHECK(Link(pCmd, pOutObjs, pOutput, silent, debug));
	}

	free(pOutObjs);
	return Y_SUCCESS;
}

long long
GetTimeNanosecond(void)
{
    LARGE_INTEGER frequency, counter;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000000LL) / frequency.QuadPart;
}

yError
Clean(Command* pCmd)
{
	DWORD dw;

	if (gbClean == false)
		return Y_SUCCESS;

	int a = DeleteDirectory(pCmd->pObjDir);
	if (a == 0)
	{
		dw = GetLastError();
		PrintErrorMessageCustom(__FUNCTION__ ,dw);
		return Y_ERROR_CLEANING;
	}
	return Y_SUCCESS;
}

#include <time.h>
/*
 * TODO: 
 * - Multithreading -> CrossPlatform = boring for this lots of lines
 * - Incremental builds -> Most of the time useless / problematic
 */
int
main(int argc, char **ppArgv)
{
	TracyCZoneNCS(mainfunc, "main function", 0x00FF00, 30, 1);

	ChefInit();
	if (ArgsCheck(argc, ppArgv) == false)
		return 1;
	Command cmd = CommandInit();

	Clean(&cmd);

	MKDIR(cmd.pObjDir);
	MKDIR(cmd.pShaderObjsDir);

	char *pCompileCommands = STR("compile_commands.json");

	/* NOTE: Get C source files in src/ recursively */
	FileList *pListCfiles = GetFileListAndObjs(&cmd, "*.c");
	if (!pListCfiles) { fprintf(stderr, "Something happened in c\n"); return 1; }

	/* NOTE: Get Cpp source files in src/ recursively */
	FileList *pListCppFiles = GetFileListAndObjs(&cmd, "*.cpp");
	if (!pListCppFiles) { fprintf(stderr, "Something happened in cpp\n"); return 1; }

	/* NOTE: Get shader files in src/shaders directory */
	FileList *pListShaders = GetFileListAndSpv(&cmd, "*");
	if (!pListShaders) { fprintf(stderr, "Something happened in shaders\n"); return 1; }

	bool bMultiThread = true;
	bool bDebug = false;

	/* NOTE: Build everything */
	yError result = Build(&cmd, pListCfiles, pListShaders, bMultiThread, bDebug);
	if ( result != Y_SUCCESS)
	{ fprintf(stderr, "%s\n", GetErrorMsg(result)); return 1; }

    /*
	 * if (ClangCompileCommandsJson(pCompileCommands))
	 * 	goto exiting;
     */
	DestroyFileList(pListCfiles);
	DestroyFileList(pListCppFiles);
	ChefDestroy();
	/* printf("String allocations: %zu\n", gChef.nbElems); */

	TracyCZoneEnd(mainfunc);
	return 0;
}
