/*****************************************************************************/
								/*WINDOWS*/
/*****************************************************************************/
#ifndef JASB_UTILS_C
#define JASB_UTILS_C

#include "jasb_utils.h"
#include "jasb_strings.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>

#include "TracyC.h"

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

int
InitFileList(FileList **ppFileList)
{
	int error = 0;
	int randomNbr = 100;

	/* NOTE: Check those allocs ? */
	FileList *pTmp = malloc(sizeof(FileList));
	pTmp->elemCount = 0;
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

void
MultiFreeImpl(void* pPtr, ...)
{
	va_list args;
	void*	pArg;

	pArg = NULL;
	va_start(args, pPtr);
	while ((pArg = va_arg(args, void*)) != NULL)
	{
		free(pArg);
	}
	va_end(args);
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
		FindFiles(ppDir[i], pRegex, pFileList->pFiles, &pFileList->elemCount);
	}
	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return pFileList;
}

FileList *
GetFileListAndSpv(Command* pCmd, const char* pRegex)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(pCmd->pShaderDir); // free this somewhere
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesAndSpv(ppDir[i], pFileList->pFiles, &pFileList->elemCount, pCmd->pShaderObjsDir, ".spv");
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
	char **ppDir = GetFilesDirIter(pCmd->pRootFolder); // free this somewhere
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesAndObj(ppDir[i], pRegex, pFileList->pFiles, &pFileList->elemCount, pCmd->pObjDir);
	}
	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return pFileList;
}

void
PrintFileList(FileList *pList)
{
	for (size_t i = 0; i < pList->elemCount; i++)
	{
		printf("File %zu: %s\n", i, pList->pFiles[i].pFullPath);
		printf("File %zu: %s\n\n", i, pList->pFiles[i].pObjName);
	}
}

void
DirectoryEnsureExists(const char* pPath)
{
    /*
	 * if (ISDIRECTORY(cmd.pObjDir) == false || DoesExist(cmd.pObjDir) == false)
	 * {
	 * 	if (MKDIR(cmd.pBuildDir) == false )
	 * 	{
	 * 		fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
	 * 		ERROR_EXIT("Mkdir: ");
	 * 	}
	 * 	if (MKDIR(cmd.pObjDir) == false )
	 * 	{
	 * 		fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
	 * 		ERROR_EXIT("Mkdir: ");
	 * 	}
	 * }
     */
}


#ifdef _WIN32
/*
 * BOOL
 * DirectoryExists(LPCTSTR szPath)
 * {
 *   DWORD dwAttrib = GetFileAttributes(szPath);
 * 
 *   return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
 * }
 */
BOOL
DeleteDirectory(const char *path)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	char searchPath[MAX_PATH];
	char filePath[MAX_PATH];

	// Create the search path (path\* to find all files and directories)
	snprintf(searchPath, MAX_PATH, "%s\\*", path);

	hFind = FindFirstFile(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	do {
		if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
			continue;

		snprintf(filePath, MAX_PATH, "%s\\%s", path, findData.cFileName);
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
			if (!DeleteDirectory(filePath)) 
			{
				FindClose(hFind);
				return FALSE;
			}
		}
		else
		{
			if (!DeleteFile(filePath))
			{
				FindClose(hFind);
				return FALSE;
			}
		}
	} while (FindNextFile(hFind, &findData) != 0);

	FindClose(hFind);
	return RemoveDirectory(path);
}

/* TODO: more robust way please, this is clunky */
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

				if (j < STACK_SIZE)
				{
					strncpy(ppDir[j], fullPath, MAX_PATH);
				}
				else
				{
					fprintf(stderr, "Stack overflow: too many directories to handle.\n");
					exit(1);
				}
			}
			if (FindNextFile(handle, &fileInfo) == 0) { break; }
		} while (1);

		FindClose(handle);
		index++;

	} while (ppDir[index][0]);
	return ppDir;
}

/* TODO: Construct absolute path instead of relative */
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
GetFilesAndSpv(const char *pPath, sFile *pFiles, size_t *pNb, const char *pBuildFolder, const char* pExtension)
{
	WIN32_FIND_DATA findFileData;
	char pSearchPath[MAX_PATH];
	sprintf(pSearchPath, "%s%s%s", pPath, SLASH, "*");
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
		if (StrIsEqual(".", findFileData.cFileName) ||
				StrIsEqual("..", findFileData.cFileName))
		{
			continue;
		}

		int lenName = strlen(findFileData.cFileName);
		sprintf(pFiles[*pNb].pObjName, "%s%s", pBuildFolder, SLASH);

		int lenObj = strlen(pFiles[*pNb].pObjName);
		int i = 0;
		for (i = 0; i < lenName; i++)
		{
			pFiles[*pNb].pObjName[i + lenObj] = findFileData.cFileName[i];
		}
		strcpy(&pFiles[*pNb].pObjName[i + lenObj], pExtension);
		strncpy(pFiles[*pNb].pFileName, findFileData.cFileName, MAX_PATH);
		sprintf(pFiles[*pNb].pFullPath, "%s%s%s", pPath, SLASH, findFileData.cFileName);
		(*pNb)++;
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
	return 0;
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
        0,
		NULL);

	lpDisplayBuf = LocalAlloc(
			LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 

    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 

	fprintf(stderr, "%s\n", (char*)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}

/* WARN: You need to free lpMsgBuf.. */
char *
GetErrorMessageStr(unsigned long dw)
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
        0,
		NULL);

	return lpMsgBuf;
}

void
PrintErrorMessage(unsigned long dw)
{
	char* buf = GetErrorMessageStr(dw);
	fprintf(stderr, "%s", buf);
	LocalFree(buf);
}

void
PrintErrorMessageCustom(const char* s, unsigned long dw)
{
	char* buf = GetErrorMessageStr(dw);
	fprintf(stderr, "\"%s\": %s", s, buf);
	LocalFree(buf);
}

#include <shlobj.h>
bool
MkdirImpl(char *pPath)
{
	char* pStr = ChefStrPath(pPath); 
	/* printf("pStr: %s\n", pStr); */
	char *finalPath = NULL;
	char pCwd[MAX_PATH];
	int error = GetCurrentDirectory(MAX_PATH, pCwd);

	if (strstr(pStr, pCwd) == NULL)
	{
		size_t lenCwd = strlen(pCwd);
		size_t lenStr = strlen(pStr);
		finalPath = malloc(sizeof(char) * (lenCwd + lenStr + 2));
		sprintf(finalPath, "%s\\%s", pCwd, pStr);
	}
	else
		finalPath = pStr;

	error = SHCreateDirectoryExA(NULL, finalPath, NULL);
	/* BOOL code = CreateDirectory(finalPath, NULL); */

	if (error != ERROR_SUCCESS)
	{
		DWORD dw = GetLastError();
		if (dw == ERROR_ALREADY_EXISTS || dw == ERROR_FILE_EXISTS)
			error = true;
		else
		{
			PrintErrorMessageCustom(finalPath, dw);
			error = false;
		}
	}
	else
		printf("Creating \"%s\"...\n", finalPath);

	ChefFree(finalPath);
	return error;
}

void
findDirWin(const char *path)
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

int
DeleteDirectory(const char* pPath)
{
	return 1;
}

/* TODO: more robust way please, this is clunky */
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

/* TODO: Construct absolute path instead of relative */
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

#endif //JASB_UTILS_C
