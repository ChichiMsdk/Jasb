/******************************************************************************
 * 						JASB. Just A Simple Builder.                          *
 ******************************************************************************
 * JASB will look for all the ".o.json" files and put them in one             *
 * compile_commands.json at the root directory. It is intended to be used with*
 * a Makefile that will compile it and then use after each build. As sed could*
 * differ from os and shell's this one should actually be portable.           *
 ******************************************************************************/

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
		.pROOTFOLDER = STR("src"),
		.pNAME = STR("yuseong"),
		.pEXTENSION = STR(EXTENSION),
		.pBUILD_DIR = STR("build"),
		.pOBJ_DIR = STR("build" SLASH "obj"),
		.pCC = STR("clang"),
		.pCPP = STR("clang++"),
		.pSRC_DIR = STR("src"),
		.pCFLAGS = STR(""),
		.pLIB_PATH = STR(""),
		.pINCLUDE_DIRS = STR(""),
		.pLIBS = STR(""),
		.pDEFINES = STR(""),
	};
	char *INCLUDEFLAG = NULL;
	char *LIBFLAG = NULL;
	char *LIBDEPEND = NULL;
	char *OPTIFLAGS = NULL;
	char *DEBUGMODEFLAGS = NULL;
	char *DEFINEFLAG = NULL;

#ifdef _WIN32 
	if ((strcmp(cmd.pCC, "clang") == 0 || (strcmp(cmd.pCC, "gcc") == 0)))
	{
		OPTIFLAGS = STR("-O3");
		DEBUGMODEFLAGS = STR("-O0");
		DEFINEFLAG = STR("-D");
		INCLUDEFLAG = STR("-I");
		LIBFLAG = STR("-L");
		LIBDEPEND = STR("-l");
		SELF_APPEND(cmd.pCFLAGS, "-Wall -Wextra -Werror");
		SELF_APPEND(cmd.pCFLAGS, " -g3");
	}
	else
	{
		OPTIFLAGS = STR("/O2");
		DEBUGMODEFLAGS = STR("/O");
		DEFINEFLAG = STR("/D");
		INCLUDEFLAG = STR("/I");
		LIBFLAG = STR("/L");
		LIBDEPEND = STR(".lib");
		SELF_APPEND(cmd.pCFLAGS, "/Wall");
		SELF_APPEND(cmd.pCFLAGS, " /Zi");
	}
	SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "PLATFORM_WINDOWS");
	ADD_FLAGS(cmd.pLIBS, LIBDEPEND, "shlwapi", "shell32", "gdi32", "winmm", "user32");

#elif __linux__
	INCLUDEFLAG = STR("-I");
	LIBFLAG = STR("-L");
	LIBDEPEND = STR("-l");
	DEFINEFLAG = STR("-D");
	OPTIFLAGS = STR("-O3");
	DEBUGMODEFLAGS = STR("-O0");
	cmd.pDEFINES = STR("-DPLATFORM_LINUX -DYGLFW3 ");
	SELF_APPEND(cmd.pCFLAGS, "-ggdb3");
	SELF_APPEND(cmd.pLIBS, "-lwayland-client -lxkbcommon -lm ");
#else
	cmd.pDEFINES = STR("");
	cmd.pLIBS = STR("");
#endif

	SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, "src", "src/core", "src/renderer/opengl");
	cmd.pCPPFLAGS = STR("");

	if ((strcmp(cmd.pCC, "clang") == 0 || (strcmp(cmd.pCC, "gcc") == 0)))
	{
		SELF_APPEND(cmd.pCFLAGS, " -fno-inline", " -fno-omit-frame-pointer");
		SELF_APPEND(cmd.pCFLAGS, " -Wno-missing-field-initializers", " -Wno-unused-but-set-variable");
		SELF_APPEND(cmd.pCFLAGS, " -Wno-uninitialized");
		SELF_APPEND(cmd.pCFLAGS, " -Wvarargs");
	}
	if (gbDebug)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "_DEBUG", "DEBUG", "YUDEBUG ");
		SELF_APPEND(cmd.pCFLAGS, " ", DEBUGMODEFLAGS);
	}
	else if (gbRelease)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "_RELEASE", "RELEASE", "YURELEASE ");
		SELF_APPEND(cmd.pCFLAGS, " ", OPTIFLAGS);
	}

	if (gbTest) SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "TESTING ");

	if (gbVulkan)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pLIB_PATH, LIBFLAG, VULKANLIB_PATH);
		SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, VULKAN_PATH);
		ADD_FLAGS(cmd.pLIBS, LIBDEPEND, VULKANLIB);
	}
	if (gbOpenGL)
	{
		ADD_FLAGS(cmd.pLIBS, LIBDEPEND, OPENGLLIB);
	}
#ifdef _WIN32
	else if (gbD3D11) { ADD_FLAGS(cmd.pLIBS, LIBDEPEND, "d3dcompiler", "d3d11", "dxgi", "dxguid"); }
	else if (gbD3D12) { }
#endif
	if (gbGLFW3)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, GLFW_PATH);
		SELF_PREPEND_WITH_FLAGS(cmd.pLIB_PATH, LIBFLAG, GLFWLIB_PATH);
		ADD_FLAGS(cmd.pLIBS, LIBDEPEND, GLFWLIB);
	}
	if (gbTracy)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "TRACY_ENABLE");
		/* CPPFLAGS =-Wno-format */
		/* Also, compile cpp flags with different struct ? */
	}
	if (gbAsan)
	{
		SELF_APPEND(cmd.pCFLAGS, " ", "-fsanitize=address");
	}
	SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, TRACY_PATH, TRACYTRACY_PATH);
	return cmd;
}

bool
StrIsEqual(const char* s1, const char* s2)
{
	if (strcmp(s1, s2) == 0)
		return true;
	else
		return false;
}

bool
ArgsCheck(int argc, char** ppArgv)
{
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

int
WildcardMatch(const char *pStr, const char *pPattern)
{
	const char *pStar = NULL;
	while (*pStr)
	{
		if (*pPattern == *pStr)
		{
			pPattern++;
			pStr++;
		}
		else if (*pPattern == '*')
			pStar = pPattern++;
		else if (pStar)
		{
			pPattern = pStar++;
			pStr++;
		}
		else
			return 0;
	}
	while (*pPattern == '*')
		pPattern++;
	return *pPattern == '\0';
}
/*
 * TODO:
 * - Add option: iterative or not
 * - Add va_args to specify multiple files manually
 */
FileList *
GetFileList(const char* pPath, const char* pRegex)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(pPath);
	for (int i = 0; ppDir[i][0]; i++)
	{
		FindFiles(ppDir[i], pRegex, pFileList->pFiles, &pFileList->nbElems);
	}
	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return pFileList;
}

FileList *
GetFileListAndObjs(Command* pCmd, const char* pRegex)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(pCmd->pROOTFOLDER); // free this somewhere
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesAndObj(ppDir[i], pRegex, pFileList->pFiles, &pFileList->nbElems, pCmd->pBUILD_DIR);
	}
	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return pFileList;
}

void
PrintFileList(FileList *pList)
{
	for (size_t i = 0; i < pList->nbElems; i++)
	{
		printf("File %zu: %s\n", i, pList->pFiles[i].pFullPath);
	}
}

int
FlushIt(char *pLine, FILE* pFd, size_t size)
{
	int count = fprintf(pFd, "%s", pLine);
	memset(pLine, 0, size);
	return count;
}

int
ConstructCompileCommandsJson(FileList *pList, const char *pName)
{
	FILE *pFd1 = fopen(pName, "w");
	if (!pFd1)
	{
		fprintf(stderr, "Cp2\n");
		perror(pName);
		DestroyFileList(pList);
		return 1;
	}
	fprintf(pFd1, "%s", CCJSON_BEGIN);
	size_t totalSize = 0;
	size_t count = 0;
	char *pLine = calloc(START_ALLOC, sizeof(char));
	int fd = 0;
	size_t i = 0;
	size_t lastOne = pList->nbElems - 1; 
	while (i < pList->nbElems)
	{
		fd = OPEN(pList->pFiles[i].pFullPath, YO_RDONLY);
		if (!fd)
			goto exiting;
		while (1)
		{
			count = READ(fd, pLine, STEPS);
			totalSize+=count;
			if (totalSize >= (START_ALLOC - STEPS))
			{
				fprintf(stderr, "Sorry line was too big. I didn't write code to realloc\n");
				errno = EOVERFLOW;
				goto exiting;
			}
			if (count < STEPS)
			{
				if (i == lastOne)
				{
					fwrite(pLine, sizeof(char), totalSize - 2, pFd1);
					fwrite("\n", sizeof(char), strlen("\n"), pFd1);
					break;
				}
				FlushIt(pLine, pFd1, totalSize);
				totalSize = 0;
				break;
			}
			if (count < 0)
				goto exiting;
		}
		CLOSE(fd);
		i++;
	}
	fprintf(pFd1, "%s", CCJSON_END);
	fclose(pFd1);
	return 0;
exiting:
	CLOSE(fd);
	fclose(pFd1);
	return 1;
}

int
ClangCompileCommandsJson(const char *pCompileCommands)
{
	FileList *pListJson = GetFileList(".", JSONREGEX);
	PrintFileList(pListJson);
	if (!pListJson)
		return 1;
	return ConstructCompileCommandsJson(pListJson, pCompileCommands);
}

int
InitFileList(FileList **ppFileList)
{
	int error = 0;
	int randomNbr = 100;

	/* NOTE: Check those allocs ? */
	FileList *pTmp = malloc(sizeof(FileList));
	pTmp->nbElems = 0;
	pTmp->pFiles = malloc(sizeof(sFile) * randomNbr);
	for (int i = 0; i < randomNbr; i++)
	{
		pTmp->pFiles[i].pDirName = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].pFileName = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].pObjName = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].pFullPath = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].maxSizeForAll = MAX_PATH;
	}
	pTmp->sizeMax = randomNbr;
	*ppFileList = pTmp;

	return error;
}

void
DestroyFileList(FileList *pFileList)
{
	for (size_t i = 0; i < pFileList->sizeMax; i++)
	{
		free(pFileList->pFiles[i].pDirName);
		free(pFileList->pFiles[i].pFileName);
		free(pFileList->pFiles[i].pFullPath);
		free(pFileList->pFiles[i].pObjName);
	}
	free(pFileList->pFiles);
	free(pFileList);
}

void
PerrorLog(char *pMsg, char *pFile, int line) 
{
	char pStr[2048];
	if (strlen(pFile) >= (2048 - strlen(" at :%d:") + strlen(pMsg)))
		perror(pMsg);
	else
	{
		sprintf(&pStr[strlen(pMsg)], "%s at %s:%d: ", pMsg, pFile, line);
		perror(pMsg);
	}
	perror(pMsg);
}

yError
CompileCmdJson(char **ppCmds, size_t nbCmds)
{
	YMB char *pName = "compile_commands.json";
    /*
	 * FILE *pFd = fopen(pName, "w");
	 * if (!pFd)
	 * {
	 * 	PERROR_LOG("CompileCmdJson");
	 * 	return Y_ERROR_JSON;
	 * }
	 * fprintf(pFd, "%s", CCJSON_BEGIN);
     */
	printf("%s", CCJSON_BEGIN);
	YMB size_t totalSize = 0;
	YMB size_t count = 0;
	size_t i = 0;
	size_t lastOne = nbCmds - 1; 
	char *pEndComa = ",\n";
	while (i < nbCmds)
	{
		printf("[%zu]: %s\n", i, ppCmds[i]);
		
		/* printf("%s%s%s,", Y_OPENCURLY, Y_DIRECTORY, PUTINQUOTE(YCWD), Y_FILE, PUTINQUOTE(ppCmds[i])) */
		if (i == lastOne)
			pEndComa = "\n";
		i++;
	}
	printf("%s", CCJSON_END);
    /*
	 * fprintf(pFd, "%s", CCJSON_END);
	 * fclose(pFd);
     */
	return Y_NO_ERROR;
}

yError
Compile(Command* pCmd, FileList* pList, char **ppOut)
{
	yError result = Y_NO_ERROR;
	if (pList->nbElems <= 0) { fprintf(stderr, "No files to compile found ..\n"); result = Y_ERROR_EMPTY; return result; }

	char *OUTFLAG;
	char *MODEFLAG;

	if ((strcmp("clang", pCmd->pCC) == 0) || (strcmp("gcc", pCmd->pCC) == 0))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}
	Command a = *pCmd;
	char **ppJson = malloc(sizeof(char *) * pList->nbElems);

	for (size_t i = 0; i < pList->nbElems; i++)
	{
		char *fname = pList->pFiles[i].pFullPath;
		char *outname = pList->pFiles[i].pObjName;
		ppJson[i] = STR("");
		SELF_APPEND_SPACE(ppJson[i], a.pCC, a.pCFLAGS, a.pDEFINES, a.pINCLUDE_DIRS, MODEFLAG, fname, OUTFLAG, outname);
		result = EXECUTE(ppJson[i], true);
		if (result != 0)
		{
			free(ppJson);
			return Y_ERROR_BUILD;
		}
	}
	/* CompileCmdJson(ppJson, pList->nbElems); */
	free(ppJson);

	// NOTE: Making the path .o files for linker
	size_t count = 0;
	size_t total = 0;

	for (size_t i = 0; i < pList->nbElems; i++)
		total += strlen(pList->pFiles[i].pObjName);

	char *pTemp = malloc(sizeof(char) * total + (pList->nbElems) + 1);
	for (size_t i = 0; i < pList->nbElems; i++)
		count += sprintf(pTemp + count, "%s ", pList->pFiles[i].pObjName);
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
Link(Command* pCmd, const char *pObj, const char *pOutName)
{
	yError result = Y_NO_ERROR;
	char *pCommand = STR("");
	Command a = *pCmd;

	char *OUTFLAG = NULL;
	char *MODEFLAG = NULL;

	if ((strcmp("clang", pCmd->pCC) == 0) || (strcmp("gcc", pCmd->pCC) == 0))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}
	SELF_APPEND_SPACE(pCommand, a.pCC, a.pCFLAGS, a.pDEFINES, pObj, OUTFLAG, pOutName, pCmd->pLIB_PATH, pCmd->pLIBS);
	result = EXECUTE(pCommand, 1);
	if (result != 0)
		return Y_ERROR_LINK;
	return result;
}

yError
Build(Command* pCmd, FileList* pListCfiles)
{
	char *pOutput = STR("");
	SELF_APPEND(pOutput, pCmd->pBUILD_DIR, SLASH, pCmd->pNAME, pCmd->pEXTENSION);
	char *pOutObjs = NULL;
	yError result = Y_NO_ERROR;

	result = Compile(pCmd, pListCfiles, &pOutObjs);
	if (result != Y_NO_ERROR)
		return result;
	result = Link(pCmd, pOutObjs, pOutput);
	if (result != Y_NO_ERROR)
		return result;
	free(pOutObjs);
	return result;
}

bool
DoesExist(const char *pPath)
{
	struct stat sb;
	int result = stat(pPath, &sb);
	if (result < 0)
	{
		PERROR_LOG("DoesExist()");
		return false;
	}
	return true;
}

/*
 * TODO: 
 * - Multithreading -> CrossPlatform = boring for this lots of lines
 * - Incremental builds -> Most of the time useless / problematic
 */
int
main(int argc, char **ppArgv)
{
	if (argc >= 2)
	{
		if (ArgsCheck(argc, ppArgv) == false)
			return 1;
	}
	ChefInit();
	Command cmd = CommandInit();
	if (ISDIRECTORY(cmd.pOBJ_DIR) == false || DoesExist(cmd.pOBJ_DIR) == false)
	{
		if (MKDIR(cmd.pBUILD_DIR) == false )
			ERROR_EXIT("Mkdir: ");
		if (MKDIR(cmd.pOBJ_DIR) == false )
			ERROR_EXIT("Mkdir: ");
	}

	YMB char *pCompileCommands = STR("compile_commands.json");

	FileList *pListCfiles = GetFileListAndObjs(&cmd, "*.c");
	if (!pListCfiles) { fprintf(stderr, "Something happened in c\n"); return 1; }

	FileList *pListCppFiles = GetFileListAndObjs(&cmd, "*.cpp");
	if (!pListCppFiles) { fprintf(stderr, "Something happened in cpp\n"); return 1; }
	yError result = Build(&cmd, pListCfiles);
	if ( result != Y_NO_ERROR)
	{ fprintf(stderr, "%s\n", GetErrorMsg(result)); return 1; }

    /*
	 * if (ClangCompileCommandsJson(pCompileCommands))
	 * 	goto exiting;
     */

	DestroyFileList(pListCfiles);
	DestroyFileList(pListCppFiles);
	ChefDestroy();
	printf("number = %zu\n", gChef.nbElems);
	return 0;

/* exiting: */
	fprintf(stderr, "Error");
	/* perror(pListJson->pFiles->pFullPath); */
	/* DestroyFileList(pListJson); */

	printf("number = %zu\n", gChef.nbElems);
	ChefDestroy();
	return 1;
}
