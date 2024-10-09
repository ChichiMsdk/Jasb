#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strsafe.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CC clang
#define MAX_SIZE_COMMAND 10000

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

char *
Compile2(const char *cc, const char *flags, FileList *pList)
{
	char *pObjs;
	for (int i = 0; i < pList->nbElems; i++)
	{
		char *fname = pList->pFiles[i].pFullPath;
		char *outname = pList->pFiles[i].pObjName;
		char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
		size_t totalCount = strlen(cc) + strlen(flags) + strlen(fname) + strlen(outname);
		snprintf(command, totalCount + 10, "%s %s -c %s -o %s", cc, flags, fname, outname);
		printf("Command: %s\n", command);
		system(command);
		free(command);
	}

	size_t count = 0;
	size_t total = 0;

	for (int i = 0; i < pList->nbElems; i++)
		total += strlen(pList->pFiles[i].pObjName);

	pObjs = malloc(sizeof(char) * total + (pList->nbElems) + 1);
	for (int i = 0; i < pList->nbElems; i++)
		count += sprintf(pObjs + count, "%s ", pList->pFiles[i].pObjName);
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
	system(command);
	free(command);
}

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
#endif
}

#define STACK_SIZE 1024

/* TODO: more robust way please, this is clunky */
char ** 
GetFilesDirIter(const char *basePath) 
{
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
}

/* TODO: Add option: iterative or not */
FileList *
GetFileList(const char *path, const char *regex, const char *pBuildFolder)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(path);
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesWin(ppDir[i], regex, pFileList->pFiles, &pFileList->nbElems, pBuildFolder);
	}
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
		}
	}
	va_end(args);
}

/*
 * NOTE: Yes I malloc and no we don't care
 * I should probably check them tho..
 */

#include "n.h"

int
main(int argc, char **ppArgv)
{
	/* findDirWin("."); */

	char *pFolder = ".";
	char *pBuildFolder = ".\\build";
	char *pName = "nomake";
	char *pExtension = ".exe";
	char *pOutput = "test.exe";
	char *pCompiler = "clang";
	char *pCompiler2 = "c++";
	char *pIncludeDirs = "src";
	char *pLibs = "-g3 -luser32";
	char *pLibPath = "";
	char *pCFlags = "-g3 -Wall -Wextra -Werror";
	char *pMacros = "PLATFORM_WINDOWS";
	char *pAsan = "";
	char *pObjs = NULL;
	char *pObjs2 = NULL;
	char *pFinal = NULL;
	FileList *CFiles = GetFileList(pFolder, "test.c", pBuildFolder);
	FileList *CPPFiles = GetFileList(pFolder, "*.cpp", pBuildFolder);

    /*
	 * for (int i = 0; i < CFiles->nbElems; i++)
	 * 	Compile(pCompiler, pCFlags, CFiles->pFiles[i].pFullPath, CFiles->pFiles[i].pObjName);
	 * for (int i = 0; i < CPPFiles->nbElems; i++)
	 * 	Compile(pCompiler2, pCFlags, CPPFiles->pFiles[i].pFullPath, CPPFiles->pFiles[i].pObjName);
     */

	pObjs = Compile2(pCompiler, pCFlags, CFiles);
	/* pObjs2 = Compile2(pCompiler2, pCFlags, CPPFiles); */
	pFinal = malloc(sizeof(char) * strlen(pOutput) + strlen(pBuildFolder) + 2);
	size_t count = sprintf(pFinal, "%s\\%s", pBuildFolder, pOutput);
	pFinal[count--] = 0;
	Link(pCompiler, pLibs, pObjs, pFinal);
	/* MakeClean("*.rdi", "*lib", "*.pdb", "*.exe", "*.exp", "*.o"); */

    /*
	 * PrintFileList(CFiles);
	 * PrintFileList(CPPFiles);
     */

	free(pFinal);
	free(pObjs);
	free(pObjs2);
	DestroyFileList(CFiles);
	DestroyFileList(CPPFiles);

	return 0;
}
