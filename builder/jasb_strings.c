#ifndef JASB_STRINGS_C
#define JASB_STRINGS_C

#include "jasb_strings.h"
#include "jasb_threads.h"

#include "TracyC.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <threads.h>

char*
ChefStrPath(const char* pPath)
{
	char *pNew = STR((char*) pPath);
	size_t size = strlen(pNew);
	size_t i = 0;
	while (i < size)
	{
		if (pNew[i] == '/' || pNew[i] == '\\')
			pNew[i] = SLASH[0];
		i++;
	}
	return pNew;
}

bool
StrIsEqual(const char* s1, const char* s2)
{
	if (strcmp(s1, s2) == 0)
		return true;
	else
		return false;
}

int
WildcardMatch(const char *pStr, const char *pPattern)
{
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
/*****************************************************************************/
/*							string memory management						 */
/*****************************************************************************/

void*
MyRealloc(void* ptr, size_t size)
{
	void* new = malloc(size);
	return new;
}

char *
ChefStrDup(char *pStr)
{
	TracyCZoneN(strDup, "ChefStrDup", 1);
	TracyCLockBeforeLock((struct __tracy_lockable_context_data *)&gChef.mutex);
	if (mtx_lock(&gChef.mutex) == thrd_error)
	{
		fprintf(stderr, "Mutex lock error\n");
		exit(1);
	}
	TracyCLockAfterLock((struct __tracy_lockable_context_data *)&gChef.mutex);
	if (gChef.nbElems >= gChef.maxSize)
	{
		gChef.ppTable = realloc(gChef.ppTable, sizeof(void *) * gChef.maxSize * 2);
		gChef.maxSize *= 2;
	}
	char *pDup = STRDUP(pStr);
	gChef.ppTable[gChef.nbElems] = pDup;
	gChef.nbElems++;
	if (mtx_unlock(&gChef.mutex) == thrd_error)
	{
		fprintf(stderr, "Mutex unlock error\n");
		exit(1);
	}
	TracyCLockAfterUnlock((struct __tracy_lockable_context_data *)&gChef.mutex);

	TracyCZoneEnd(strDup);
	return pDup;
}

#define CHEF_ALLOC 100

void
ChefInit(void)
{
	gChef.ppTable = malloc(sizeof(void*) * CHEF_ALLOC);
	gChef.nbElems = 0;
	gChef.maxSize = CHEF_ALLOC;

	MutexInit(&gChef.mutex, mtx_plain);
	MutexInit(&gChef.mutexRealloc, mtx_plain);
}

void
ChefDestroy(void)
{
	TracyCZoneN(chefDestroy, __FUNCTION__, 1);
	for (size_t i = 0; i < gChef.nbElems; i++)
	{
		/* printf("Freeing: \"%s\" at %p\n", (char*)gChef.ppTable[i], gChef.ppTable[i]); */
		free(gChef.ppTable[i]);
	}
	mtx_destroy(&gChef.mutex);
	mtx_destroy(&gChef.mutexRealloc);
	TracyCZoneEnd(chefDestroy);
}

void
ChefFree(void *pPtr)
{
	TracyCZoneN(ChefFree, "ChefFree", 1);
	for (size_t i = 0; i < gChef.nbElems; i++)
	{
		if (gChef.ppTable[i] == pPtr)
		{
			TracyCZoneN(freeNull, "Free and NULL", 1);
			free(gChef.ppTable[i]);
			gChef.ppTable[i] = NULL;
			TracyCZoneEnd(freeNull);
		}
	}
	free(pPtr);
	TracyCZoneEnd(ChefFree);
}

/* FIXME: Sizes are (1 * size) but char could be > 1 ! */
/* WARN: Check if NULL !!!!! */
void *
ChefRealloc(void *pPtr, size_t size)
{
	TracyCZoneN(reall, "ChefRealloc", 1);

	TracyCLockBeforeLock((struct __tracy_lockable_context_data *)&gChef.mutexRealloc);
	if (mtx_lock(&gChef.mutexRealloc) == thrd_error)
	{
		fprintf(stderr, "Mutex lock error\n");
		exit(1);
	}
	TracyCLockAfterLock((struct __tracy_lockable_context_data *)&gChef.mutexRealloc);

	size_t i = 0;
	for (i = 0; i < gChef.nbElems; i++)
	{
		if (gChef.ppTable[i] == pPtr)
		{
			gChef.ppTable[i] = realloc(gChef.ppTable[i], size);
			break;
		}
	}
	if (mtx_unlock(&gChef.mutexRealloc) == thrd_error)
	{
		fprintf(stderr, "Mutex unlock error\n");
		exit(1);
	}
	TracyCZoneEnd(reall);

	TracyCLockAfterUnlock((struct __tracy_lockable_context_data *)&gChef.mutexRealloc);
	return gChef.ppTable[i];
}

#define SPACELEN 1
char *
ChefStrAppendSpaceImpl(char *pDst, ...)
{
	TracyCZoneN(strappend, __FUNCTION__, 1);
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
	TracyCZoneEnd(strappend);
	return pDst;
}

char *
ChefStrAppendImpl(char *pDst, ...)
{
	TracyCZoneN(strappend, __FUNCTION__, 1);
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
	TracyCZoneEnd(strappend);
	return pDst;
}

char *
ChefStrAppendWithFlagsImpl(char *pDst, char *pFlag, ...)
{
	if (!pFlag)
		return NULL;
	TracyCZoneN(strappend, __FUNCTION__, 1);
	va_list args;
	char *pArg = NULL;
	va_start(args, pFlag);
	size_t flagSize = strlen(pFlag);
	/* size_t dstSize = strlen(pDst); */
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		if (strlen(pArg) <= 0)
			continue;
		size_t argSize = strlen(pArg);
		size_t dstSize = strlen(pDst);

		pDst = ChefRealloc(pDst, dstSize + argSize + 1 + flagSize + SPACELEN);

		memcpy(&pDst[dstSize], pArg, argSize);
		memcpy(&pDst[dstSize + argSize], pFlag, flagSize);

		pDst[dstSize + argSize + flagSize] = ' ';
		pDst[dstSize + argSize + flagSize + 1] = 0;
	}
	va_end(args);
	TracyCZoneEnd(strappend);
	return pDst;
}

char *
ChefStrPrependWithFlagsImpl(char *pDst, char *pFlag, ...)
{
	if (!pFlag)
		return NULL;
	TracyCZoneN(prepend, __FUNCTION__, 1);
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

		pDst = ChefRealloc(pDst, dstSize + srcSize + 1 + flagSize + SPACELEN);

		memcpy(&pDst[dstSize], pFlag, flagSize);
		memcpy(&pDst[dstSize + flagSize], pArg, srcSize);

		pDst[dstSize + srcSize + flagSize] = ' ';
		pDst[dstSize + srcSize + flagSize + 1] = 0;
	}
	va_end(args);
	TracyCZoneEnd(prepend);
	return pDst;
}

char *
ChefStrSurroundImpl(char *pDst, char *pSurround)
{
	TracyCZoneN(surround, __FUNCTION__, 1);
	size_t dstSize = strlen(pDst);
	size_t surroundSize = strlen(pSurround);
	char *tmp = ChefStrDup(pDst);
	tmp = ChefRealloc(tmp, dstSize + (surroundSize * 2) + 1);
	memcpy(tmp, pSurround, surroundSize);
	memcpy(&tmp[surroundSize], pDst, dstSize);
	memcpy(&tmp[surroundSize + dstSize], pSurround, surroundSize);
	tmp[(surroundSize * 2) + dstSize] = 0;
	TracyCZoneEnd(surround);
	return tmp;
}

#endif //JASB_STRINGS_C
