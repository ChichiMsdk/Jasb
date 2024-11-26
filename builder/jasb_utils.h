#ifndef JASB_UTILS_H
#define JASB_UTILS_H

#include <stdbool.h>

#define YMB [[maybe_unused]]

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
	char *pPch;
}sFile;

typedef struct FileList
{
	char **ppList;
	sFile *pFiles;
	size_t elemCount;
	size_t sizeMax;
}FileList;

typedef struct Command
{
	char *pOutputName;
	char *pRootFolder;
	char *pExtension;
	char *pBuildDir;
	char *pObjDir;
	char *pCc;
	char *pCpp;
	char *pGlslc;
	char *pSrcDir;
	char *pShaderDir;
	char *pShaderObjsDir;
	char *pIncludeDirs;
	char *pLibPath;
	char *pLibs;
	char *pCflags;
	char *pDefines;
	char *pCppFlags;
}Command;

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <corecrt_io.h>
	#include <shlwapi.h>
	#include <fcntl.h>
	#include <strsafe.h>

	#define SLASH "\\"
	#define EXTENSION ".exe"

	void ErrorExit(char* lpszMsg, unsigned long dw);
	BOOL DeleteDirectory(const char *path);

	bool MkdirImpl(char* pStr);
	void PerrorLog(char *pMsg, char *pFile, int line);
	#define PERROR_LOG(str) PerrorLog(str, __FILE__, __LINE__)

	#define YO_RDONLY _O_RDONLY
	#define OPEN(a, b) _open(a, b)
	#define CLOSE(a) _close(a)
	#define READ(a, b, c) _read(a, b, c)
	#define MKDIR(str) MkdirImpl(str)
	#define ISDIRECTORY(str) PathIsDirectory(str)
	#define ERROR_EXIT(str) ErrorExit(str, GetLastError())

#elif __linux__
	#include <ftw.h>
	#include <dirent.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <unistd.h>

	#define MAX_PATH 320
	char pError[1124];

	void ErrorExit(char *pMsg);
	bool PathIsDirectory(const char *pPath);

	#define SLASH "/"
	#define EXTENSION ""
	#define YO_RDONLY O_RDONLY
	#define OPEN(a, b) open(a, b)
	#define CLOSE(a) close(a)
	#define READ(a, b, c) read(a, b, c)
	#define MKDIR(str) MkdirImpl(str)
	#define ISDIRECTORY(str) PathIsDirectory(str)
	#define ERROR_EXIT(str) ErrorExit(str)
#endif

#define JASBAPPEND 1
#define JASBPREPEND 2
#define CCJSON_BEGIN "[\n"
#define CCJSON_END "]"
#define STEPS 3000
#define START_ALLOC 10240
#define MAX_SIZE_COMMAND 10000
#define STACK_SIZE 1024

int		IsValidDirImpl(const char *pStr, unsigned long attr, ...);
#define IsValidDir(...) IsValidDirImpl(__VA_ARGS__, NULL)

int		InitFileList(FileList **ppFileList);
void	DestroyFileList(FileList *pFileList);

void	MultiFreeImpl(void* pPtr, ...);
#define MultiFree(...) MultiFreeImpl(__VA_ARGS__, NULL)

bool		DoesExist(const char *pPath);
char**		GetFilesDirIter(const char *pBasePath);
void		FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb);
int			GetFilesAndSpv(const char *pPath, sFile *pFiles, size_t *pNb, const char *pBuildFolder, const char* pExtension);
int			GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder);
FileList*	GetFileList(const char* pPath, const char* pRegex);

FileList*	GetFileListAndSpv(Command* pCmd, const char* pRegex);
FileList*	GetFileListAndObjs(Command* pCmd, const char* pRegex);
void		DirectoryEnsureExists(const char* pPath);
void		PrintErrorMessage(unsigned long dw);
void		PrintFileList(FileList *pList);

void		PerrorLog(char *pMsg, char *pFile, int line);


#endif //JASB_UTILS_H
