/******************************************************************************
 * 						JASB. Just A Simple Builder.                          *
 ******************************************************************************
 * JASB will look for all the ".o.json" files and put them in one             *
 * compile_commands.json at the root directory. It is intended to be used with*
 * a Makefile that will compile it and then use after each build. As sed could*
 * differ from os and shell's this one should actually be portable.           *
 ******************************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef enum yError {
	Y_NO_ERROR = 0x00,

	Y_ERROR_EMPTY = 0x01,
	Y_ERROR_UNKNOWN = 0x02,
	Y_ERROR_EXEC = 0x03,
	Y_ERROR_BUILD = 0x04,
	Y_ERROR_LINK = 0x05,
	Y_ERROR_JSON = 0x06,
	Y_MAX_ERROR
}yError;

char *pErrorMsg[] = {
	"No error.",
	"List is empty.",
	"Unknown error.",
	"Execution error.",
	"Build couldn't finish.",
	"Link couldn't finish.",
	"Json couldn't not be created.",
	"Clean function did not succeed.",
};

#define GetErrorMsg(a) pErrorMsg[a]

char *gpPrintHelper[] = {
	"{ ",
	"\"directory\": ",
	"\"file\": ",
	"\"output\": ",
	"\"arguments\": [",
	"]}",
};

#define Y_OPENCURLY gpPrintHelper[0]
#define Y_DIRECTORY gpPrintHelper[1]
#define Y_FILE gpPrintHelper[2]
#define Y_OUTPUT gpPrintHelper[3]
#define Y_BEGIN_ARGUMENTS gpPrintHelper[4]
#define Y_CLOSE_ARGUMENTS gpPrintHelper[5]
#define YCOMA ","

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define EXECUTE(x, y) ExecuteImpl(x, y)

int
ExecuteImpl(const char* pCommand, int dry)
{
	int code = 0;
	if (dry == 1)
		printf("%s\n", pCommand);
	else
		code = system(pCommand);
	return code;
}

#define YMB [[maybe_unused]]

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <strsafe.h>
	#include <corecrt_io.h>
	#include <shlwapi.h>

	void ErrorExit(char* lpszMsg, unsigned long dw);
	bool MkdirImpl(char* pStr);

#pragma comment(lib, "shlwapi.lib")
	#define YO_RDONLY _O_RDONLY
	#define OPEN(a, b) _open(a, b)
	#define CLOSE(a) _close(a)
	#define READ(a, b, c) _read(a, b, c)
	#define STRDUP(str) _strdup(str)
	#define MKDIR(str) MkdirImpl(str)
	#define ISDIRECTORY(str) PathIsDirectory(str)
	#define ERROR_EXIT(str) ErrorExit(str, GetLastError())

	#define SLASH "\\"
	#define EXTENSION ".exe"
	#define TRACY_PATH "C:\\Lib\\tracy-0.11.1\\public"
	#define TRACYTRACY_PATH "C:\\Lib\\tracy-0.11.1\\public\\tracy"

	#define GLFW_PATH "C:\\Lib\\glfw\\include" 
	#define GLFWLIB_PATH "C:\\Lib\\glfw\\lib-vc2022"
	#define GLFWLIB "gflw3_mt"

	#define VULKAN_PATH "C:/VulkanSDK/1.3.275.0/Include"
	#define VULKANLIB_PATH "C:/VulkanSDK/1.3.275.0/Lib"
	#define VULKANLIB "vulkan-1"
	#define OPENGLLIB "opengl32"

	void PerrorLog(char *pMsg, char *pFile, int line);
	#define PERROR_LOG(str) PerrorLog(str, __FILE__, __LINE__)
	bool DoesExist(const char *pPath);

#elif __linux__
	#include <stdarg.h>
	#include <ftw.h>
	#include <dirent.h>
	#include <errno.h>
	#include <unistd.h>
	#include <sys/stat.h>

	#define MAX_PATH 320
	char pError[1124];


	void ErrorExit(char *pMsg);
	bool PathIsDirectory(const char *pPath);

	#define YO_RDONLY O_RDONLY
	#define OPEN(a, b) open(a, b)
	#define CLOSE(a) close(a)
	#define READ(a, b, c) read(a, b, c)
	#define STRDUP(str) strdup(str)
	#define MKDIR(str) MkdirImpl(str)
	#define ISDIRECTORY(str) PathIsDirectory(str)
	#define ERROR_EXIT(str) ErrorExit(str)

	#define SLASH "/"
	#define EXTENSION ""

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

#define JASBAPPEND 1
#define JASBPREPEND 2
#define CCJSON_BEGIN "[\n"
#define CCJSON_END "]"
#define STEPS 3000
#define START_ALLOC 10240
#define MAX_SIZE_COMMAND 10000
#define STACK_SIZE 1024

/* NOTE: YES ! All on the heap. */
typedef struct sFile
{
	char *pFileName;
	char *pObjName;
	char *pDirName;

	/* NOTE: I'm insecure */
	int maxSizeForAll;

	/* TODO: make this absolute */
	char *pFullPath;
}sFile;

typedef struct FileList
{
	char **ppList;
	sFile *pFiles;
	size_t nbElems;
	size_t sizeMax;
}FileList;

typedef struct Command
{
	char *pNAME;
	char *pROOTFOLDER;
	char *pEXTENSION;
	char *pBUILD_DIR;
	char *pOBJ_DIR;
	char *pCC;
	char *pCPP;
	char *pSRC_DIR;
	char *pINCLUDE_DIRS;
	char *pLIB_PATH;
	char *pLIBS;
	char *pCFLAGS;
	char *pDEFINES;
	char *pCPPFLAGS;
}Command;

/*****************************************************************************/
/*							string memory management						 */
/*****************************************************************************/

typedef struct MemChef
{
	void **ppTable;
	size_t nbElems;
	size_t maxSize;
}MemChef;

MemChef gChef = {0};

char *
ChefStrDup(char *pStr)
{
	if (gChef.nbElems >= gChef.maxSize)
	{
		gChef.ppTable = realloc(gChef.ppTable, gChef.maxSize * 2);
		gChef.maxSize *= 2;
	}
	char *pDup = STRDUP(pStr);
	gChef.ppTable[gChef.nbElems] = pDup;
	gChef.nbElems++;
	return pDup;
}

#define CHEF_ALLOC 100

void
ChefInit(void)
{
	gChef.ppTable = malloc(sizeof(void*) * CHEF_ALLOC);
	gChef.nbElems = 0;
	gChef.maxSize = CHEF_ALLOC;
}

void
ChefDestroy(void)
{
	for (size_t i = 0; i < gChef.nbElems; i++)
	{
		/* printf("Freeing: \"%s\" at %p\n", (char*)gChef.ppTable[i], gChef.ppTable[i]); */
		free(gChef.ppTable[i]);
	}
}

void
ChefFree(void *pPtr)
{
	for (size_t i = 0; i < gChef.nbElems; i++)
	{
		if (gChef.ppTable[i] == pPtr)
		{
			free(gChef.ppTable[i]);
			gChef.ppTable[i] = NULL;
		}
	}
	free(pPtr);
}

void *
ChefRealloc(void *pPtr, size_t size)
{
	size_t i = 0;
	for (i = 0; i < gChef.nbElems; i++)
	{
		if (gChef.ppTable[i] == pPtr)
		{
			gChef.ppTable[i] = realloc(gChef.ppTable[i], size);
			break;
		}
	}
	return gChef.ppTable[i];
}

#define SPACELEN 1
char *
ChefStrAppendSpaceImpl(char *pDst, ...)
{
	va_list args;
	va_start(args, pDst);
	char *pArg = NULL;
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		if (strlen(pArg) <= 0)
			continue;
		size_t dstSize = strlen(pDst);
		size_t srcSize = strlen(pArg);
		pDst = ChefRealloc(pDst, dstSize + srcSize + 1 + SPACELEN);
		memcpy(&pDst[dstSize], pArg, srcSize);
		pDst[dstSize + srcSize] = ' ';
		pDst[dstSize + srcSize + 1] = 0;
	}
	va_end(args);
	return pDst;
}

char *
ChefStrAppendImpl(char *pDst, ...)
{
	va_list args;
	va_start(args, pDst);
	char *pArg = NULL;
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		size_t dstSize = strlen(pDst);
		size_t srcSize = strlen(pArg);
		pDst = ChefRealloc(pDst, dstSize + srcSize + 1);
		memcpy(&pDst[dstSize], pArg, srcSize);
		pDst[dstSize + srcSize] = 0;
	}
	va_end(args);
	return pDst;
}

char *
ChefStrAppendWithFlagsImpl(char *pDst, char *pFlag, ...)
{
	if (!pFlag)
		return NULL;
	va_list args;
	char *pArg = NULL;
	va_start(args, pFlag);
	size_t flagSize = strlen(pFlag);
	/* size_t dstSize = strlen(pDst); */
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		if (strlen(pArg) <= 0)
			continue;
		size_t srcSize = strlen(pArg);
		size_t dstSize = strlen(pDst);
		pDst = ChefRealloc(pDst, dstSize + srcSize + 1 + flagSize + SPACELEN);
		memcpy(&pDst[dstSize], pFlag, flagSize);
		memcpy(&pDst[dstSize + flagSize], pArg, srcSize);
		pDst[dstSize + srcSize + flagSize] = ' ';
		pDst[dstSize + srcSize + flagSize + 1] = 0;
	}
	va_end(args);
	return pDst;
}

char *
ChefStrPrependWithFlagsImpl(char *pDst, char *pFlag, ...)
{
	if (!pFlag)
		return NULL;
	va_list args;
	char *pArg = NULL;
	va_start(args, pFlag);
	size_t flagSize = strlen(pFlag);
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		if (strlen(pArg) <= 0)
			continue;
		size_t srcSize = strlen(pArg);
		size_t dstSize = strlen(pDst);
		char *tmp = malloc(dstSize + 1);

		pDst = ChefRealloc(pDst, dstSize + srcSize + 1 + flagSize + SPACELEN);

		memcpy(&pDst[dstSize], pFlag, flagSize);
		memcpy(&pDst[dstSize + flagSize], pArg, srcSize);

		pDst[dstSize + srcSize + flagSize] = ' ';
		pDst[dstSize + srcSize + flagSize + 1] = 0;

		free(tmp);
	}
	va_end(args);
	return pDst;
}

char *
ChefStrSurroundImpl(char *pDst, char *pSurround)
{
	size_t dstSize = strlen(pDst);
	size_t surroundSize = strlen(pSurround);
	char *tmp = ChefStrDup(pDst);
	tmp = ChefRealloc(tmp, dstSize + (surroundSize * 2) + 1);
	memcpy(tmp, pSurround, surroundSize);
	memcpy(&tmp[surroundSize], pDst, dstSize);
	memcpy(&tmp[surroundSize + dstSize], pSurround, surroundSize);
	tmp[(surroundSize * 2) + dstSize] = 0;
	return tmp;
}

#define STR(a) ChefStrDup(a)
#define SELF_APPEND(a, ...) a = ChefStrAppendImpl(a, __VA_ARGS__, NULL)
#define SELF_APPEND_SPACE(a, ...) a = ChefStrAppendSpaceImpl(a, __VA_ARGS__, NULL)

#define APPEND(a, ...) ChefStrAppendImpl(a, __VA_ARGS__, NULL)
#define APPEND_SPACE(a, ...) ChefStrAppendSpaceImpl(a, __VA_ARGS__, NULL)

#define PUTINQUOTE(a) ChefStrSurroundImpl(a, "\"")

#define SELF_PREPEND_WITH_FLAGS(dst, flags, ...) dst = ChefStrPrependWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)
#define SELF_APPEND_WITH_FLAGS(dst, flags, ...) dst = ChefStrAppendWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)

#define PREPEND_WITH_FLAGS(dst, flags, ...) ChefStrPrependWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)
#define APPEND_WITH_FLAGS(dst, flags, ...) ChefStrAppendWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)

#define ADD_FLAGS(dst, flags, ...) \
	do { \
		if (strcmp(flags, "-lib") == 0) { \
			SELF_APPEND_WITH_FLAGS(dst, flags, __VA_ARGS__); \
		} \
		if (strcmp(flags, "-l") == 0) { \
			SELF_PREPEND_WITH_FLAGS(dst, flags, __VA_ARGS__); \
		} \
	} while(0);


void MakeCleanImpl(void *none, ...);
int IsValidDirImpl(const char *pStr, unsigned long attr, ...);

#define IsValidDir(...) IsValidDirImpl(__VA_ARGS__, NULL)
#define MakeClean(...) MakeCleanImpl(__VA_ARGS__, NULL)

int InitFileList(FileList **ppFileList);
void DestroyFileList(FileList *pFileList);
int WildcardMatch(const char *pStr, const char *pPattern);

/* TODO: Construct absolute path instead of relative */
void FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb);

/* TODO: more robust way please, this is clunky */
char **GetFilesDirIter(const char *pBasePath);
int GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder);
FileList *GetFileList(const char* pPath, const char* pRegex);
FileList *GetFileListAndObjs(Command* pCmd, const char* pRegex);

#define JSONREGEX "*.o.json"
void PrintFileList(FileList *pList);
int FlushIt(char *pLine, FILE* pFd, size_t size);
int ConstructCompileCommandsJson(FileList *pList, const char *pName);
int ClangCompileCommandsJson(const char *pCompileCommands);

/*****************************************************************************/
								/*WINDOWS*/
/*****************************************************************************/
#ifdef WIN32

/*
 * BOOL
 * DirectoryExists(LPCTSTR szPath)
 * {
 *   DWORD dwAttrib = GetFileAttributes(szPath);
 * 
 *   return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
 * }
 */
char ** 
GetFilesDirIter(const char *pBasePath)
{
	char **ppDir = malloc(sizeof(char *) * STACK_SIZE);
	for (int i = 0; i < STACK_SIZE; i++)
	{
		ppDir[i] = malloc(sizeof(char) * MAX_PATH);
		memset(ppDir[i], 0, MAX_PATH);
	}
	int index = 0;

	strncpy(ppDir[index], pBasePath, MAX_PATH);
	char fullPath[1024];
	WIN32_FIND_DATA fileInfo;
	int j = 0;
	do
	{
		char *currentPath = ppDir[index];
		char searchPath[1024];

		sprintf(searchPath, "%s%s*", currentPath, SLASH);
		HANDLE handle = FindFirstFile(searchPath, &fileInfo);
		if (handle == INVALID_HANDLE_VALUE)
			fprintf(stderr, "Unable to open directory: Error(%lu)\n", GetLastError());
		do
		{
			if (IsValidDir(fileInfo.cFileName, fileInfo.dwFileAttributes) == true)
			{
				j++;
				memset(fullPath, 0, sizeof(fullPath) / sizeof(fullPath[0]));
				sprintf(fullPath, "%s%s%s", currentPath, SLASH, fileInfo.cFileName);

				if (j < STACK_SIZE) strncpy(ppDir[j], fullPath, MAX_PATH);
				else fprintf(stderr, "Stack overflow: too many directories to handle.\n");
			}
			if (FindNextFile(handle, &fileInfo) == 0) { break; }
		} while (1);

		FindClose(handle);
		index++;

	} while (ppDir[index][0]);
	return ppDir;
}

void
FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb)
{
	WIN32_FIND_DATA findFileData;
	char pSearchPath[MAX_PATH];
	sprintf(pSearchPath, "%s%s%s", pPath, SLASH, pRegex);
	HANDLE hFind = FindFirstFile(pSearchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		if (dw != ERROR_FILE_NOT_FOUND)
			ErrorExit(pSearchPath, dw);
		return ; 
	}
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	do
	{
		strncpy(pFiles[*pNb].pFileName, findFileData.cFileName, MAX_PATH);
		sprintf(pFiles[*pNb].pFullPath, "%s%s%s", pPath, SLASH, findFileData.cFileName);
		(*pNb)++;
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

int
GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder)
{
	WIN32_FIND_DATA findFileData;
	char pSearchPath[MAX_PATH];
	sprintf(pSearchPath, "%s%s%s", pPath, SLASH, pRegex);
	HANDLE hFind = FindFirstFile(pSearchPath, &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		if (dw != ERROR_FILE_NOT_FOUND)
			ErrorExit(pSearchPath, dw);
		return 1; 
	}
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	do
	{
		int lenName = strlen(findFileData.cFileName);
		sprintf(pFiles[*pNb].pObjName, "%s%s", pBuildFolder, SLASH);
		int lenObj = strlen(pFiles[*pNb].pObjName);
		for (int i = 0; i < lenName; i++)
		{
			if (findFileData.cFileName[i] == '.')
			{
				pFiles[*pNb].pObjName[i + lenObj] = findFileData.cFileName[i];
				pFiles[*pNb].pObjName[++i + lenObj] = 'o';
				pFiles[*pNb].pObjName[++i + lenObj] = 0;
				break;
			}
			pFiles[*pNb].pObjName[i + lenObj] = findFileData.cFileName[i];
		}
		strncpy(pFiles[*pNb].pFileName, findFileData.cFileName, MAX_PATH);
		sprintf(pFiles[*pNb].pFullPath, "%s%s%s", pPath, SLASH, findFileData.cFileName);
		(*pNb)++;
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
	return 0;
}

int
IsValidDirImpl(const char *pStr, unsigned long dwAttr, ...)
{
	va_list args;
	va_start(args, dwAttr);
	char *pArgStr = NULL;
	while ((pArgStr = va_arg(args, char *)) != NULL)
	{
		if (!strcmp(pArgStr, pStr))
		{
			va_end(args);
			return false;
		}
	}

	/* NOTE: Excluded directories */
	if (!StrIsEqual(pStr, ".") && !StrIsEqual(pStr, ".."))
	{
		if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
			return true;
	}
	return false;
}

void
ErrorExit(char* lpszFunction, unsigned long dw) 
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}

bool
MkdirImpl(char *pStr)
{
	return CreateDirectory(pStr, NULL);
}

void
findDirWin([[maybe_unused]] const char *path)
{
	WIN32_FIND_DATA findData;
	char newPath[MAX_PATH];
	sprintf(newPath, "%s%s*", path, SLASH);
	HANDLE hFind = FindFirstFile(newPath, &findData);
	memset(newPath, 0, sizeof(newPath) / sizeof(newPath[0]));

	if (hFind == INVALID_HANDLE_VALUE) { printf("FindFirstDir failed (%lu)\n", GetLastError()); return; } 
	do
	{
		if (IsValidDir(findData.cFileName, findData.dwFileAttributes))
		{
			/* printf("Dir: %s\n", findData.cFileName); */
			sprintf(newPath, "%s%s%s", path, SLASH, findData.cFileName);
			printf("Dir: %s\n", newPath);
			findDirWin(newPath);
		}
	}
	while (FindNextFile(hFind, &findData) != 0);
}

#endif

/*****************************************************************************/
								/*LINUX*/
/*****************************************************************************/
#ifdef __linux__
char ** 
GetFilesDirIter(const char *pBasePath) 
{
	DIR *pdStream;
	struct dirent *pdEntry;
	char pFullPath[1024];
	char pSearchPath[1024];
	int index = 0;
	int j = 0;

	char **ppDir = malloc(sizeof(char *) * STACK_SIZE);
	for (int i = 0; i < STACK_SIZE; i++)
	{
		ppDir[i] = malloc(sizeof(char) * MAX_PATH);
		memset(ppDir[i], 0, MAX_PATH);
	}
	strncpy(ppDir[index], pBasePath, MAX_PATH);
	do
	{
		char *pCurrentPath = ppDir[index];
		sprintf(pSearchPath, "%s/.", pCurrentPath);
		sprintf(pError, "Couldn't open '%s'", pSearchPath);
		if ((pdStream = opendir(pSearchPath)) == NULL) { perror(pError); return NULL; }
		while ((pdEntry = readdir(pdStream)) != NULL)
		{
			if (IsValidDir(pdEntry->d_name, pdEntry->d_type, ".git") == true)
			{
				j++;
				memset(pFullPath, 0, sizeof(pFullPath) / sizeof(pFullPath[0]));
				sprintf(pFullPath, "%s/%s", pCurrentPath, pdEntry->d_name);
				if (j < STACK_SIZE) 
					strncpy(ppDir[j], pFullPath, MAX_PATH);
				else
				{
					fprintf(stderr, "Stack overflow:" "too many directories to handle:" " %s\n", pFullPath);
					goto panic;
				}
			}
		}
		closedir(pdStream);
		index++;
	} while (ppDir[index][0]);
	return ppDir;

panic:
	closedir(pdStream);
	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return NULL;
}

void
FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb)
{
	DIR *pdStream;
	struct dirent *pdEntry;
	char pSearchPath[1024];

	sprintf(pSearchPath, "%s/.", pPath);
	sprintf(pError, "Couldn't open '%s'", pSearchPath);
	if ((pdStream = opendir(pSearchPath)) == NULL) 
	{ 
		perror(pError); 
		return ; 
	}
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	while ((pdEntry = readdir(pdStream)) != NULL)
	{
		if (pdEntry->d_type == DT_REG)
		{
			if (WildcardMatch(pdEntry->d_name, pRegex))
			{
				strncpy(pFiles[*pNb].pFileName, pdEntry->d_name, MAX_PATH);
				sprintf(pFiles[*pNb].pFullPath, "%s/%s", pPath, pdEntry->d_name);
				(*pNb)++;
			}
		}
	}
	closedir(pdStream);
}

int
GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder)
{
	DIR *pdStream;
	struct dirent *pdEntry;
	char pSearchPath[1024];

	sprintf(pSearchPath, "%s/.", pPath);
	sprintf(pError, "Couldn't open '%s'", pSearchPath);
	if ((pdStream = opendir(pSearchPath)) == NULL) 
	{ 
		perror(pError); 
		return 1; 
	}
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	while ((pdEntry = readdir(pdStream)) != NULL)
	{
		if (pdEntry->d_type == DT_REG)
		{
			if (WildcardMatch(pdEntry->d_name, pRegex))
			{
				int lenName = strlen(pdEntry->d_name);
				sprintf(pFiles[*pNb].pObjName, "%s/", pBuildFolder);
				int lenObj = strlen(pFiles[*pNb].pObjName);
				for (int i = 0; i < lenName; i++)
				{
					if (pdEntry->d_name[i] == '.')
					{
						pFiles[*pNb].pObjName[i + lenObj] = pdEntry->d_name[i];
						pFiles[*pNb].pObjName[++i + lenObj] = 'o';
						pFiles[*pNb].pObjName[++i + lenObj] = 0;
						break;
					}
					pFiles[*pNb].pObjName[i + lenObj] = pdEntry->d_name[i];
				}
				strncpy(pFiles[*pNb].pFileName, pdEntry->d_name, MAX_PATH);
				sprintf(pFiles[*pNb].pFullPath, "%s/%s", pPath, pdEntry->d_name);
				(*pNb)++;
			}
		}
	}
	closedir(pdStream);
	return 0;
}

int
IsValidDirImpl(const char *pStr, unsigned long attr, ...)
{
	va_list args;
	va_start(args, attr);
	char *pArgStr = NULL;
	while ((pArgStr = va_arg(args, char *)) != NULL)
	{
		if (!strcmp(pArgStr, pStr))
		{
			va_end(args);
			return false;
		}
	}
	va_end(args);
	if (strcmp(pStr, ".") && strcmp(pStr, ".."))
	{
		if ((unsigned char)attr == DT_DIR)
			return true;
	}
	return false;
}

bool
PathIsDirectory(const char *pPath)
{
	struct stat sb;
	int result = stat(pPath, &sb);
	if (result < 0)
	{
		PERROR_LOG("PathIsDirectoy()");
		return false;
	}
	if (S_ISDIR(sb.st_mode))
		return true;
	return false;
}

bool
MkdirImpl(char *pStr)
{
	if (mkdir(pStr, 0700) == 0)
		return true;
	return false;
}

void
ErrorExit(char *pMsg) 
{
	perror(pMsg);
	exit(1);
}
#endif


