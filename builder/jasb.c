/******************************************************************************
 |                      JASB. Just A "Simple" Builder.                        |
 ******************************************************************************
 | JASB will look for all the ".o.json" files and put them in one             |
 | compile_commands.json at the root directory. It is intended to be used with|
 | a Makefile that will compile it and then use after each build. As sed could|
 | differ from os and shell's this one should actually be portable.           |
 ******************************************************************************/

#include "jasb.h"
#include "jasb_execute.h"
#include "jasb_utils.h"
#include "jasb_strings.h"
#include "jasb_errors.h"
#include "jasb_execute.h"

#include "TracyC.h"

MemChef gChef = {0};

#pragma comment(lib, "shlwapi.lib")

bool gbDebug = true;
bool gbRelease = false;

bool gbTracy = false;
bool gbAsan = true;
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
		.pCc = STR("clang"),
		.pCpp = STR("clang++"),
		.pGlslc = STR("glslc"),
		.pSrcDir = STR("src"),
		.pShaderDir = STR("src" SLASH "shaders"),
		.pShaderObjsDir = STR("build" SLASH "obj" SLASH "shaders"),
		.pCflags = STR(""),
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
		SELF_APPEND(cmd.pCflags, "-Wall -Wextra");
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
		LIBFLAG = STR("/L");
		LIBDEPEND = STR(".lib");
		SELF_APPEND(cmd.pCflags, "/Wall");
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
	printf("\n");
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

	char *OUTFLAG	= NULL;
	char *MODEFLAG	= NULL;
	if (StrIsEqual("clang", pCmd->pCc) || StrIsEqual("gcc", pCmd->pCc))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
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

	for (size_t i = 0; i < pList->elemCount; i++)
	{
		track += strlen(pList->pFiles[i].pObjName);
		if (track >= total)
		{ fprintf(stderr, "Too long linker command %zu\n", track); exit(1); }

		count += sprintf(pTemp + count, "%s ", pList->pFiles[i].pObjName);

		char*	pFilePath	= pList->pFiles[i].pFullPath;
		char*	pOutputName	= pList->pFiles[i].pObjName;

		ppJson[i] = STR("");
		char*	pPchFlag = NULL;
		char*	pTimeReportFlag = NULL;

		if (StrIsEqual(pFilePath, "src"SLASH"renderer"SLASH"vulkan"SLASH"vulkan_loader.c"))
		{
			pPchFlag = "-include-pch src"SLASH"pch"SLASH"stb_image_pch.h.pch"
				" -include-pch src"SLASH"pch"SLASH"windows_pch.h.pch";

			pTimeReportFlag = "-ftime-trace";
		}
		else
		{
			pTimeReportFlag = "";
			pTimeReportFlag = "-ftime-trace";
			pPchFlag = "-include-pch src"SLASH"pch"SLASH"windows_pch.h.pch";
		}

		SELF_APPEND_SPACE(ppJson[i], a.pCc, pTimeReportFlag, a.pCflags, a.pDefines, a.pIncludeDirs, pPchFlag);
		SELF_APPEND_SPACE(ppJson[i], MODEFLAG, pFilePath, OUTFLAG, pOutputName);
		/* SELF_APPEND_SPACE(ppJson[i], a.pCC, a.pCFLAGS, a.pDEFINES, a.pINCLUDE_DIRS, MODEFLAG, fname, OUTFLAG, outname); */

		if (bMultiThread)
		{
			pArgs[i].pThreadName = pFilePath;
			pArgs[i].silent = silent;
			pArgs[i].debug	= debug;
			pArgs[i].id = i;
			pArgs[i].finished = false;
			pArgs[i].pCmd = ppJson[i];
			thrd_create(&pThreads[i], ThreadExec, &pArgs[i]);
		}
		else
		{
			result = EXECUTE(ppJson[i], pFilePath, silent, debug);
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
Link(Command* pCmd, const char *pObj, const char *pOutName, bool silent, bool debug)
{
	char *pCommand = STR("");
	Command a = *pCmd;

	char*	OUTFLAG		= NULL;
	char*	MODEFLAG	= NULL;
	char*	LLD_FLAGS	= STR("-fuse-ld=lld");
	/* char*	LLD_FLAGS	= STR(""); */

	if (StrIsEqual("clang", pCmd->pCc) || StrIsEqual("gcc", pCmd->pCc))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}

	/* SELF_APPEND_SPACE(pCommand, a.pCC, a.pCFLAGS, a.pDEFINES, pObj, OUTFLAG, pOutName, pCmd->pLIB_PATH, pCmd->pLIBS); */
	SELF_APPEND_SPACE(pCommand, a.pCc, LLD_FLAGS, a.pCflags, a.pDefines);
	SELF_APPEND_SPACE(pCommand, pObj, OUTFLAG, pOutName, pCmd->pLibPath, pCmd->pLibs);

	TracyCZoneNC(linkingpart, "Link", 0x00001, 1);
	JASB_CHECK(EXECUTE(pCommand, pOutName, silent, debug));
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
	printf("\n");

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
		ShadersBuild(pCmd, pShadersList, silent);

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

/*
 * TODO: 
 * - Multithreading -> CrossPlatform = boring for this lots of lines
 * - Incremental builds -> Most of the time useless / problematic
 */
int
main(int argc, char **ppArgv)
{
	TracyCZoneNC(mainfunc, "main function", 0x00FF00, 1);

	if (ArgsCheck(argc, ppArgv) == false)
		return 1;

	ChefInit();
	Command cmd = CommandInit();

    /*
	 * DWORD dw;
	 * int a = DeleteDirectory(cmd.pBUILD_DIR);
	 * if (a == 0)
	 * {
	 * 	dw = GetLastError();
	 * }
     */

	if (ISDIRECTORY(cmd.pObjDir) == false || DoesExist(cmd.pObjDir) == false)
	{
		if (MKDIR(cmd.pBuildDir) == false )
		{
			fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
			ERROR_EXIT("Mkdir: ");
		}
		if (MKDIR(cmd.pObjDir) == false )
		{
			fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
			ERROR_EXIT("Mkdir: ");
		}
	}

	if (ISDIRECTORY(cmd.pShaderObjsDir) == false || DoesExist(cmd.pShaderObjsDir) == false)
	{
		if (MKDIR(cmd.pShaderObjsDir) == false )
		{
			fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
			ERROR_EXIT("Mkdir: ");
		}
	}

	YMB char *pCompileCommands = STR("compile_commands.json");

	FileList *pListCfiles = GetFileListAndObjs(&cmd, "*.c");
	if (!pListCfiles) { fprintf(stderr, "Something happened in c\n"); return 1; }

	FileList *pListCppFiles = GetFileListAndObjs(&cmd, "*.cpp");
	if (!pListCppFiles) { fprintf(stderr, "Something happened in cpp\n"); return 1; }

	FileList *pListShaders = GetFileListAndSpv(&cmd, "*");
	if (!pListShaders) { fprintf(stderr, "Something happened in shaders\n"); return 1; }

	bool bMultiThread = false;
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
	printf("String allocations: %zu\n", gChef.nbElems);

	TracyCZoneEnd(mainfunc);

	return 0;
}
