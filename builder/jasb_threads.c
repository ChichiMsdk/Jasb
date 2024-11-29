#include "jasb_threads.h"

#include <stdio.h>
#include <stdlib.h>

void
MutexInitImpl(mtx_t* pMtx, int type, const char* pFunc, const char* pFile, int line)
{
	if (mtx_init(pMtx, type) != thrd_success)
	{
		fprintf(stderr, "Mutex init error %s in %s:%d\n", pFunc, pFile, line);
		fprintf(stderr, "Exiting..\n");
		exit(1);
	}
}

void
MutexLockImpl(mtx_t* pMtx, const char* pFunc, const char* pFile, int line)
{
	if (mtx_lock(pMtx) != thrd_success)
	{
		fprintf(stderr, "Mutex lock error %s in %s:%d\n", pFunc, pFile, line);
		fprintf(stderr, "Exiting..\n");
		exit(1);
	}
}

void
MutexUnlockImpl(mtx_t* pMtx, const char* pFunc, const char* pFile, int line)
{
	if (mtx_unlock(pMtx) != thrd_success)
	{
		fprintf(stderr, "Mutex unlock error %s in %s:%d\n", pFunc, pFile, line);
		fprintf(stderr, "Exiting..\n");
		exit(1);
	}
}
