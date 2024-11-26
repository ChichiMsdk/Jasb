#ifndef JASB_EXECUTE_H
#define JASB_EXECUTE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct threadStruct
{
	char*			pCmd;
	char*			pThreadName;
	char*			pTargetPath;
	char*			pDependencyPath;
	bool			silent;
	bool			debug;
	bool			noExec;
	int				id;
	_Atomic _Bool	finished;

}threadStruct;

int	ThreadExecShaders(void* args);
int ThreadExec(void* args);
int ExecuteImpl(const char* pCommand, const char* pFile, const char* pTarget, bool dry, bool debug);
int IsOutdated(const char* pDependency, const char* pTarget);

#define EXECUTE(x, str, str2, y, z) ExecuteImpl(x, str, str2, y, z)

#endif //JASB_EXECUTE_H

