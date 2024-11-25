/*****************************************************************************/
/*							string memory management						 */
/*****************************************************************************/
#ifndef JASB_STRINGS_H
#define JASB_STRINGS_H

#include <stdbool.h>

#ifdef _WIN32
#define STRDUP(str) _strdup(str)
#elif __linux__
#define STRDUP(str) strdup(str)
#endif

bool StrIsEqual(const char* s1, const char* s2);
int WildcardMatch(const char *pStr, const char *pPattern);

typedef struct MemChef
{
	void **ppTable;
	size_t nbElems;
	size_t maxSize;
} MemChef;

extern	MemChef gChef;

#define CHEF_ALLOC 100
#define SPACELEN 1

char*	ChefStrDup(char *pStr);

void	ChefInit(void);
void	ChefDestroy(void);
void	ChefFree(void *pPtr);
void*	ChefRealloc(void *pPtr, size_t size);

char*	ChefStrAppendSpaceImpl(char *pDst, ...);
char*	ChefStrAppendImpl(char *pDst, ...);
char*	ChefStrAppendWithFlagsImpl(char *pDst, char *pFlag, ...);
char*	ChefStrPrependWithFlagsImpl(char *pDst, char *pFlag, ...);
char*	ChefStrSurroundImpl(char *pDst, char *pSurround);

#define STR(a) ChefStrDup(a)
#define SELF_APPEND(a, ...) a = ChefStrAppendImpl(a, __VA_ARGS__, NULL)
#define SELF_APPEND_SPACE(a, ...) a = ChefStrAppendSpaceImpl(a, __VA_ARGS__, NULL)

#define APPEND(a, ...) ChefStrAppendImpl(a, __VA_ARGS__, NULL)
#define APPEND_SPACE(a, ...) ChefStrAppendSpaceImpl(a, __VA_ARGS__, NULL)

#define PUTINQUOTE(a) ChefStrSurroundImpl(a, "\"")

#define SELF_PREPEND_WITH_FLAGS(dst, flags, ...) dst = ChefStrPrependWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)
#define SELF_APPEND_WITH_FLAGS(dst, flags, ...) dst = ChefStrAppendWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)

#define PREPEND_WITH_FLAGS(dst, flags, ...) ChefStrPrependWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)
#define APPEND_WITH_FLAGS(dst, flags, ...) ChefStrAppendWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)

#define ADD_FLAGS(dst, flags, ...) \
	do { \
		if (strcmp(flags, "-lib") == 0) { \
			SELF_APPEND_WITH_FLAGS(dst, flags, __VA_ARGS__); \
		} \
		if (strcmp(flags, "-l") == 0) { \
			SELF_PREPEND_WITH_FLAGS(dst, flags, __VA_ARGS__); \
		} \
	} while(0);

#endif //JASB_STRINGS_H
