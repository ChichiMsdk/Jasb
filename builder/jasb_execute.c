#ifndef JASB_EXECUTE_C
#define JASB_EXECUTE_C

#include "jasb_execute.h"
#include "jasb_threads.h"
#include <fcntl.h>
#include <sys/types.h>
#include "jasb.h"

static _Atomic uint32_t gCOUNT = 0;

/* TODO: Handle errors */
int
IsOutdated(const char* pDependency, const char* pTarget)
{
    struct stat target_stat;
	struct stat dep_stat;

    if (stat(pTarget, &target_stat) != 0) 
		return 1;

    if (stat(pDependency, &dep_stat) != 0) 
		return 0;

    return dep_stat.st_mtime > target_stat.st_mtime;
}

int
ThreadExecShaders(void* args)
{
	int				code		= 0;
	threadStruct*	argsStruct	= (threadStruct*)(args) ;
	char			buf[100];

	sprintf(buf, "Thread for Shaders %d", argsStruct->id);

	TracyCSetThreadName(buf)
	TracyCZoneNC(threading, __FUNCTION__, 0x8800ff, activate);

	gCOUNT++;

	TracyCMessage(argsStruct->pThreadName, strlen(argsStruct->pThreadName));

	if (IsOutdated(argsStruct->pDependencyPath, argsStruct->pTargetPath))
	{
		if (argsStruct->debug == true)
		{
			MutexLock(argsStruct->pMutex);
			printf("%s\n", argsStruct->pCmd);
			MutexUnlock(argsStruct->pMutex);

			argsStruct->finished = true;
			TracyCZoneEnd(threading);
			thrd_exit(code);
		}

		if (argsStruct->silent == false)
		{
			MutexLock(argsStruct->pMutex);
			printf("%s\n", argsStruct->pCmd);
			MutexUnlock(argsStruct->pMutex);
		}

		TracyCZoneNC(syscall, "System call", 0x8888ff, activate);

		char line[5000];

		/* TODO: Error checking */
		FILE* pFd = _popen(argsStruct->pCmd, "r");

		MutexLock(argsStruct->pMutex);

		int val = 0;

		do
		{
			val = fscanf(pFd, "%s", line);
			printf("val %d\n", val);
			/* printf("%s", line); */
		}
		while (val > 0);

		code = _pclose(pFd);

		MutexUnlock(argsStruct->pMutex);

		/* code = system(argsStruct->pCmd); */

		TracyCZoneEnd(syscall);
	}
	else
	{
        /*
		 * if (argsStruct->silent == false)
		 * 	printf("%s is up to date.\n", argsStruct->pTargetPath);
         */
	}
	argsStruct->finished = true;
	TracyCZoneEnd(threading);
	thrd_exit(code);
}

int
ThreadExec(void* args)
{
	int				code		= 0;
	threadStruct*	argsStruct	= (threadStruct*)(args) ;
	char			buf[100];
	if (argsStruct->noExec == false)
	{
		argsStruct->finished = true;
		thrd_exit(code);
		return code;
	}

	sprintf(buf, "Thread %d", argsStruct->id);

	TracyCSetThreadName(buf)
	TracyCZoneNC(threading, __FUNCTION__, 0x8800ff, activate);

	gCOUNT++;

	/* TracyCMessage(argsStruct->pThreadName, strlen(argsStruct->pThreadName)); */
	
	if (IsOutdated(argsStruct->pDependencyPath, argsStruct->pTargetPath))
	{
		if (argsStruct->debug == true)
		{
			MutexLock(argsStruct->pMutex);
			printf("%s\n", argsStruct->pCmd);
			MutexUnlock(argsStruct->pMutex);

			argsStruct->finished = true;
			TracyCZoneEnd(threading);
			thrd_exit(code);
		}
		if (argsStruct->silent == false)
		{
			MutexLock(argsStruct->pMutex);
			printf("%s\n", argsStruct->pCmd);
			MutexUnlock(argsStruct->pMutex);
		}

		TracyCZoneNC(syscall, "System call", 0x8888ff, activate);

		char line[5000];

		/* TODO: Error checking */
		FILE* pFd = _popen(argsStruct->pCmd, "r");

		MutexLock(argsStruct->pMutex);

		int val = 0;

		do
		{
			val = fscanf(pFd, "%s", line);
			printf("val: %d\n", val);
			/* printf("%s", line); */
		}
		while (val > 0);

		/* printf("\n"); */

		code = _pclose(pFd);

		MutexUnlock(argsStruct->pMutex);

		/* code = system(argsStruct->pCmd); */
		TracyCZoneEnd(syscall);
	}
	else
	{
        /*
		 * if (argsStruct->silent == false)
		 * 	printf("%s is up to date.\n", argsStruct->pTargetPath);
         */
	}

	argsStruct->finished = true;

	TracyCZoneEnd(threading);
	thrd_exit(code);
}

int
ExecuteImpl(const char* pCommand, const char* pFile, const char* pTarget, bool silent, bool debug)
{
	TracyCZoneNC(execute, __FUNCTION__, 0x8800ff, 1);
	/* TracyCMessage(pFile, strlen(pFile)); */

	int code = 0;
	/* if (IsOutdated(pFile, pTarget)) */
	{
		if (debug == true)
		{
			printf("%s\n", pCommand);
			TracyCZoneEnd(execute);
			return code;
		}
		if (silent == false)
			printf("%s\n", pCommand);
		code = system(pCommand);
	}
	/* else */
	{
        /*
		 * if (silent == false)
		 * 	printf("%s is up to date.\n", pFile);
         */
	}

	TracyCZoneEnd(execute);
	return code;
}

#endif //JASB_EXECUTE_C
