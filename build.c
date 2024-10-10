#ifdef WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <strsafe.h>
#elif __linux__
	#include <stdarg.h>
	#include <ftw.h>
	#include <dirent.h>
	#include <errno.h>
	#define MAX_PATH 320
	char pError[1124];
#endif // WIN32
	
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CC clang
#define MAX_SIZE_COMMAND 10000

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
	int nbElems;
	int sizeMax;
}FileList;

#ifdef WIN32
void
ErrorExit(LPCTSTR lpszFunction, DWORD dw) 
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
#elif __linux__
void
ErrorExit(char *pMsg) 
{
	perror(pMsg);
	exit(1);
}
#endif // WIN32

char *
Compile2(const char *cc, const char *flags, FileList *pList)
{
	if (pList->nbElems <= 0)
	{
		fprintf(stderr, "No files to compile found ..\n");
		return NULL;
	}
	char *pObjs;
	for (int i = 0; i < pList->nbElems; i++)
	{
		char *fname = pList->pFiles[i].pFullPath;
		char *outname = pList->pFiles[i].pObjName;
		char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
		size_t totalCount = strlen(cc) + strlen(flags) + strlen(fname) + strlen(outname);
		snprintf(command, totalCount + 10, "%s %s -c %s -o %s", cc, flags, fname, outname);
		printf("Command: %s\n", command);
		/* system(command); */
		free(command);
	}

	size_t count = 0;
	size_t total = 0;

	for (int i = 0; i < pList->nbElems; i++)
		total += strlen(pList->pFiles[i].pObjName);

	pObjs = malloc(sizeof(char) * total + (pList->nbElems) + 1);
	for (int i = 0; i < pList->nbElems; i++)
		count += sprintf(pObjs + count, "%s ", pList->pFiles[i].pObjName);
    /*
	 * printf("Count: %zu\n", count);
	 * printf("%zu + %d = %zu\n", total, pList->nbElems, total + pList->nbElems);
     */
	pObjs[--count] = 0;
	return pObjs;
}

void
Compile(const char *cc, const char *flags, const char *fname, const char *outname)
{
	char *pObjs;
	char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
	size_t totalCount = strlen(cc) + strlen(flags) + strlen(fname) + strlen(outname);
	snprintf(command, totalCount + 10, "%s %s -c %s -o %s", cc, flags, fname, outname);
	printf("Command: %s\n", command);
	system(command);
	free(command);
}

void
Link(const char *cc, const char *flags, const char *obj, const char *outname)
{
	char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
	size_t totalCount = strlen(cc) + strlen(flags) + strlen(obj) + strlen(outname);
	snprintf(command, totalCount + 7, "%s %s %s -o %s", cc, flags, obj, outname);
	printf("Command: %s\n", command);
	/* system(command); */
	free(command);
}

#ifdef WIN32
int
isValidDirWin(const char *str, DWORD dwAttr)
{
	if (strcmp(str, ".") && strcmp(str, ".."))
	{
		if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
			return 1;
	}
	return 0;
}
#elif __linux__

#define isValidDirWin(...) isValidDirWinImpl(__VA_ARGS__, NULL)

int
isValidDirWinImpl(const char *pStr, int attr, ...)
{
	va_list args;
	va_start(args, attr);
	char *pArgStr = NULL;
	while ((pArgStr = va_arg(args, char *)) != NULL)
	{
		if (!strcmp(pArgStr, pStr))
		{
			va_end(args);
			return 0;
		}
	}
	if (strcmp(pStr, ".") && strcmp(pStr, ".."))
	{
		if ((unsigned char)attr == DT_DIR)
			return va_end(args), 1;
	}
	va_end(args);
	return 0;
}
#endif // WIN32

void
findDirWin(const char *path)
{
#ifdef WIN32
	WIN32_FIND_DATA findData;
	char newPath[MAX_PATH];
	sprintf(newPath, "%s\\*", path);
	/* printf("newPath1: %s\n", newPath); */
	HANDLE hFind = FindFirstFile(newPath, &findData);
	memset(newPath, 0, sizeof(newPath) / sizeof(newPath[0]));

	if (hFind == INVALID_HANDLE_VALUE) { printf("FindFirstDir failed (%lu)\n", GetLastError()); return; } 
	do
	{
		if (isValidDirWin(findData.cFileName, findData.dwFileAttributes))
		{
			/* printf("Dir: %s\n", findData.cFileName); */
			sprintf(newPath, "%s\\%s", path, findData.cFileName);
			printf("Dir: %s\n", newPath);
			findDirWin(newPath);
		}
	}
	while (FindNextFile(hFind, &findData) != 0);
#elif __linux__
#endif
}

void
DestroyFileList(FileList *pFileList)
{
	for (int i = 0; i < pFileList->sizeMax; i++)
	{
		free(pFileList->pFiles[i].pDirName);
		free(pFileList->pFiles[i].pFileName);
		free(pFileList->pFiles[i].pObjName);
		free(pFileList->pFiles[i].pFullPath);
	}
	free(pFileList->pFiles);
	free(pFileList);
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

int WildcardMatch(const char *pStr, const char *pPattern);
/* TODO: Construct absolute path instead of relative */
void
GetFilesWin(const char *pPath, const char *pRegex, sFile *pFiles, int *pNb, const char *pBuildFolder)
{
#ifdef WIN32
	WIN32_FIND_DATA findFileData;
	char pSearchPath[MAX_PATH];
	sprintf(pSearchPath, "%s\\%s", pPath, pRegex);
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
		int lenName = strlen(findFileData.cFileName);
		sprintf(pFiles[*pNb].pObjName, "%s\\", pBuildFolder);
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
		sprintf(pFiles[*pNb].pFullPath, "%s\\%s", pPath, findFileData.cFileName);
		(*pNb)++;
		/* printf("%s\n", fullPath); //findFileData.cFileName); */
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
#elif __linux__
	DIR *pdStream;
	struct dirent *pdEntry;
	char pFullPath[1024];
	char pSearchPath[1024];
	int index = 0;
	int j = 0;

	sprintf(pSearchPath, "%s/.", pPath);
	sprintf(pError, "Couldn't open '%s'", pSearchPath);
	if ((pdStream = opendir(pSearchPath)) == NULL) { perror(pError); return ; }
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
#endif
}

#define STACK_SIZE 1024

/* TODO: more robust way please, this is clunky */
char ** 
GetFilesDirIter(const char *pBasePath) 
{
#ifdef WIN32
	char **ppDir = malloc(sizeof(char *) * STACK_SIZE);
	for (int i = 0; i < STACK_SIZE; i++)
	{
		ppDir[i] = malloc(sizeof(char) * MAX_PATH);
		memset(ppDir[i], 0, MAX_PATH);
	}
	int index = 0;

	strncpy(ppDir[index], basePath, MAX_PATH);
	char fullPath[1024];
	WIN32_FIND_DATA fileInfo;
	int j = 0;
	do
	{
		char *currentPath = ppDir[index];
		char searchPath[1024];

		sprintf(searchPath, "%s\\*", currentPath);
		HANDLE handle = FindFirstFile(searchPath, &fileInfo);
		if (handle == INVALID_HANDLE_VALUE)
			fprintf(stderr, "Unable to open directory: Error(%lu)\n", GetLastError());
		do
		{
			if (isValidDirWin(fileInfo.cFileName, fileInfo.dwFileAttributes))
			{
				j++;
				memset(fullPath, 0, sizeof(fullPath) / sizeof(fullPath[0]));
				sprintf(fullPath, "%s\\%s", currentPath, fileInfo.cFileName);

				if (j < STACK_SIZE) strncpy(ppDir[j], fullPath, MAX_PATH);
				else fprintf(stderr, "Stack overflow: too many directories to handle.\n");
			}
			if (FindNextFile(handle, &fileInfo) == 0) { break; }
		} while (1);

		FindClose(handle);
		index++;

	} while (ppDir[index][0]);
	return ppDir;
#elif __linux__
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
			if (isValidDirWin(pdEntry->d_name, pdEntry->d_type, ".git"))
			{
				j++;
				memset(pFullPath, 0, sizeof(pFullPath) / sizeof(pFullPath[0]));
				sprintf(pFullPath, "%s/%s", pCurrentPath, pdEntry->d_name);
				/* printf("pFullPath[%d] at index[%d]: %s\n", j, index, pFullPath); */
				if (j < STACK_SIZE) strncpy(ppDir[j], pFullPath, MAX_PATH);
				else
				{
					fprintf(stderr, "Stack overflow:"
							"too many directories to handle:"
							" %s\n", pFullPath);
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
#endif // WIN32
}

/* TODO: Add option: iterative or not */
FileList *
GetFileList(const char *path, const char *regex, const char *pBuildFolder)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(path); // free this somewhere
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesWin(ppDir[i], regex, pFileList->pFiles, &pFileList->nbElems, pBuildFolder);
	}

	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return pFileList;
}

void
PrintFileList(FileList *pList)
{
	for (int i = 0; i < pList->nbElems; i++)
	{
		printf("File %d: %s\n", i, pList->pFiles[i].pFullPath);
		printf("Obj %d: %s\n", i, pList->pFiles[i].pObjName);
	}
}

#define MakeClean(...) MakeCleanImpl(NULL, __VA_ARGS__, NULL)

/* TODO: Specify a "cleaning" directory */
void
MakeCleanImpl(void *none, ...)
{
	(void)none;
	va_list args;
	va_start(args, none);

	char *str;
	while ((str = va_arg(args, char *)) != NULL)
	{
		if (str)
		{
#ifdef WIN32
			WIN32_FIND_DATA findFileData;
			HANDLE hFind = FindFirstFile(str, &findFileData);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				DWORD dw = GetLastError();
				if (dw != ERROR_FILE_NOT_FOUND)
					ErrorExit(str, dw);
				return ; 
			}
			do{
				printf("Removing %s..\n", findFileData.cFileName);
				remove(findFileData.cFileName);
			}
			while (FindNextFile(hFind, &findFileData) != 0);
#elif __linux__
#endif // WIN32
		}
	}
	va_end(args);
}

/*
 * NOTE: Yes I malloc and no we don't care
 * I should probably check them tho..
 */

int
WildcardMatch(const char *pStr, const char *pPattern)
{
	const char *p = pPattern;
	const char *s = pStr;
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

int
main(int argc, char **ppArgv)
{
	/* findDirWin("."); */

	char *pFolder = "."; char *pBuildFolder = "build";
	char *pName = "nomake"; char *pExtension = ".exe";
	char *pOutput = "test.exe"; char *pCompiler = "clang";
	char *pCompiler2 = "c++"; char *pIncludeDirs = "src";
	char *pLibs = "-g3 -luser32"; char *pLibPath = "";
	char *pCFlags = "-g3 -Wall -Wextra -Werror"; char *pMacros = "PLATFORM_WINDOWS";
	char *pAsan = ""; char *pObjs = NULL; char *pObjs2 = NULL; char *pFinal = NULL;

	char *str = "src/main.c";
	char *pattern = "*.c";
	char **ppDir = NULL;

	FileList *CFiles = GetFileList(pFolder, "*.c", pBuildFolder);
	FileList *CPPFiles = GetFileList(pFolder, "*.cpp", pBuildFolder);

    /*
	 * for (int i = 0; i < CFiles->nbElems; i++)
	 * 	Compile(pCompiler, pCFlags, CFiles->pFiles[i].pFullPath, CFiles->pFiles[i].pObjName);
	 * for (int i = 0; i < CPPFiles->nbElems; i++)
	 * 	Compile(pCompiler2, pCFlags, CPPFiles->pFiles[i].pFullPath, CPPFiles->pFiles[i].pObjName);
     */

	pObjs = Compile2(pCompiler, pCFlags, CFiles);
	if (!pObjs)
		pObjs = "";
	/* pObjs2 = Compile2(pCompiler2, pCFlags, CPPFiles); */
	pFinal = malloc(sizeof(char) * strlen(pOutput) + strlen(pBuildFolder) + 2);
	size_t count = sprintf(pFinal, "%s/%s", pBuildFolder, pOutput);
	pFinal[count--] = 0;
	Link(pCompiler, pLibs, pObjs, pFinal);
	/* MakeClean("*.rdi", "*lib", "*.pdb", "*.exe", "*.exp", "*.o"); */

    /*
	 * PrintFileList(CFiles);
	 * PrintFileList(CPPFiles);
     */

	free(pFinal);
	if (strlen(pObjs) > 0)
		free(pObjs);
	free(pObjs2);
	DestroyFileList(CFiles);
	DestroyFileList(CPPFiles);

	return 0;
}
