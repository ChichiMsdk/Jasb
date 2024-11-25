#include "jasb_execute.h"

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
	TracyCZoneNC(threading, __FUNCTION__, 0x8800ff, 1);

	gCOUNT++;

	TracyCMessage(argsStruct->pThreadName, strlen(argsStruct->pThreadName));

	if (IsOutdated(argsStruct->pDependencyPath, argsStruct->pTargetPath))
	{
		if (argsStruct->debug == true)
		{
			printf("%s\n\n", argsStruct->pCmd);
			argsStruct->finished = true;
			TracyCZoneEnd(threading);
			thrd_exit(code);
		}

		if (argsStruct->silent == false)
			printf("%s\n\n", argsStruct->pCmd);

		TracyCZoneNC(syscall, "System call", 0x8888ff, 1);
		code = system(argsStruct->pCmd);
		TracyCZoneEnd(syscall);
	}
	else
		if (argsStruct->silent == false)
			printf("%s is up to date.\n", argsStruct->pTargetPath);
	

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

	sprintf(buf, "Thread %d", argsStruct->id);

	TracyCSetThreadName(buf)
	TracyCZoneNC(threading, __FUNCTION__, 0x8800ff, 1);

	gCOUNT++;

	TracyCMessage(argsStruct->pThreadName, strlen(argsStruct->pThreadName));
	
	if (argsStruct->debug == true)
	{
		printf("%s\n\n", argsStruct->pCmd);
		argsStruct->finished = true;
		TracyCZoneEnd(threading);
		thrd_exit(code);
	}

	if (argsStruct->silent == false)
		printf("%s\n\n", argsStruct->pCmd);

	TracyCZoneNC(syscall, "System call", 0x8888ff, 1);
	code = system(argsStruct->pCmd);
	TracyCZoneEnd(syscall);

	argsStruct->finished = true;

	TracyCZoneEnd(threading);

	thrd_exit(code);
}

int
ExecuteImpl(const char* pCommand, const char* pFile, bool silent, bool debug)
{
	TracyCZoneNC(execute, __FUNCTION__, 0x8800ff, 1);
	TracyCMessage(pFile, strlen(pFile));

	int code = 0;
	if (debug == true)
	{
		printf("%s\n\n", pCommand);
		TracyCZoneEnd(execute);
		return code;
	}
	if (silent == false)
		printf("%s\n\n", pCommand);
	code = system(pCommand);

	TracyCZoneEnd(execute);
	return code;
}
