#ifndef JASB_THREADS_H
#define JASB_THREADS_H

#include <threads.h>

void	MutexUnlockImpl(mtx_t* pMtx, const char* pFunc, const char* pFile, int line);
void	MutexLockImpl(mtx_t* pMtx, const char* pFunc, const char* pFile, int line);
void	MutexInitImpl(mtx_t* pMtx, int type, const char* pFunc, const char* pFile, int line);

#define MutexInit(a, b) MutexInitImpl(a, b, __FUNCTION__, __FILE__, __LINE__)
#define MutexLock(a) MutexLockImpl(a, __FUNCTION__, __FILE__, __LINE__)
#define MutexUnlock(a) MutexUnlockImpl(a, __FUNCTION__, __FILE__, __LINE__)

#endif // JASB_THREADS_H
