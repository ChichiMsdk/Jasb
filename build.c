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


void
compile(const char *cc, const char *flags, const char *fname, const char *outname)
{
	char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
	size_t totalCount = strlen(cc) + strlen(flags) + strlen(fname) + strlen(outname);
	printf("wrote: %d\n", snprintf(command, totalCount + 10, "%s %s -c %s -o %s", cc, flags, fname, outname));
	printf("Command: %s\n", command);
	system(command);
	free(command);
}

void
link(const char *cc, const char *flags, const char *obj, const char *outname)
{
	char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
	size_t totalCount = strlen(cc) + strlen(flags) + strlen(obj) + strlen(outname);
	printf("wrote: %d\n", snprintf(command, totalCount + 7, "%s %s %s -o %s", cc, flags, obj, outname));
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

typedef struct FileList
{
	char *ppList[MAX_PATH];
	int nbElems;
	int sizeMax;
}FileList;

/* TODO: Construct absolute path instead of relative */
void
GetFilesWin(const char *path, const char *regex) 
{
#ifdef WIN32
	WIN32_FIND_DATA findFileData;
	char searchPath[MAX_PATH];
	char fullPath[MAX_PATH];
	sprintf(searchPath, "%s\\%s", path, regex);
	HANDLE hFind = FindFirstFile(searchPath, &findFileData);

	/* if (hFind == INVALID_HANDLE_VALUE) { printf("FindFirstFile failed (%lu)\n", GetLastError()); return; }  */
	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		if (dw != ERROR_FILE_NOT_FOUND)
			ErrorExit(searchPath, dw);
		exit(1); 
	}
	do
	{
		sprintf(fullPath, "%s\\%s", path, findFileData.cFileName);
		printf("%s\n", fullPath); //findFileData.cFileName);
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
#endif
}

#define STACK_SIZE 1024

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
		if (handle == INVALID_HANDLE_VALUE) { fprintf(stderr, "Unable to open directory: %s from %s\nError(%lu)\n",
				currentPath, searchPath, GetLastError()); memset(currentPath, 0, MAX_PATH); continue; }
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
GetFileList(const char *path, const char *regex)
{
	FileList *pList;
	char **ppFiles;
	char **ppDir = GetFilesDirIter(path);
	for (int i = 0; ppDir[i][0]; i++)
		GetFilesWin(ppDir[i], regex);
	return pList;
}

int
main(int argc, char **ppArgv)
{
    /*
	 * compile("clang", "-Wall -Wextra -Werror", "test.c", "build/test.o");
	 * link("clang", "", "build/test.o", "test.exe");
     */
	/* findDirWin("."); */

	char *pFolder = ".";
	char *pBuildFolder = "build";
	char *pName = "nomake";
	char *pExtension = ".exe";
	char *pCompiler = "clang";
	char *pIncludeDirs = "src";
	char *pLibs = "user32";
	char *pLibPath = "";
	char *pCFlags[] = {"-g", "Wall", "Wextra", "Werror"};
	char *pMacros = "PLATFORM_WINDOWS";
	char *pAsan = "";
	FileList *CFiles = GetFileList(pFolder, "*.c");
	FileList *CPPFiles = GetFileList(pFolder, "*.cpp");

	return 0;
}
