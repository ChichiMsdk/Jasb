#include "jasb_compile_commands.h"

#include <stdio.h>
#include <string.h>

int
FlushIt(char *pLine, FILE* pFd, size_t size)
{
	int count = fprintf(pFd, "%s", pLine);
	memset(pLine, 0, size);
	return count;
}

int
ConstructCompileCommandsJson(FileList *pList, const char *pName)
{
	FILE *pFd1 = fopen(pName, "w");
	if (!pFd1)
	{
		fprintf(stderr, "Cp2\n");
		perror(pName);
		DestroyFileList(pList);
		return 1;
	}
	fprintf(pFd1, "%s", CCJSON_BEGIN);
	size_t totalSize = 0;
	size_t count = 0;
	char *pLine = calloc(START_ALLOC, sizeof(char));
	int fd = 0;
	size_t i = 0;
	size_t lastOne = pList->elemCount - 1; 
	while (i < pList->elemCount)
	{
		fd = OPEN(pList->pFiles[i].pFullPath, YO_RDONLY);
		if (!fd)
			goto exiting;
		while (1)
		{
			count = READ(fd, pLine, STEPS);
			totalSize+=count;
			if (totalSize >= (START_ALLOC - STEPS))
			{
				fprintf(stderr, "Sorry line was too big. I didn't write code to realloc\n");
				errno = EOVERFLOW;
				goto exiting;
			}
			if (count < STEPS)
			{
				if (i == lastOne)
				{
					fwrite(pLine, sizeof(char), totalSize - 2, pFd1);
					fwrite("\n", sizeof(char), strlen("\n"), pFd1);
					break;
				}
				FlushIt(pLine, pFd1, totalSize);
				totalSize = 0;
				break;
			}
			if (count < 0)
				goto exiting;
		}
		CLOSE(fd);
		i++;
	}
	fprintf(pFd1, "%s", CCJSON_END);
	fclose(pFd1);
	return 0;
exiting:
	CLOSE(fd);
	fclose(pFd1);
	return 1;
}

int
ClangCompileCommandsJson(const char *pCompileCommands)
{
	FileList *pListJson = GetFileList(".", JSONREGEX);
	PrintFileList(pListJson);
	if (!pListJson)
		return 1;
	return ConstructCompileCommandsJson(pListJson, pCompileCommands);
}

yError
CompileCmdJson(char **ppCmds, size_t nbCmds)
{
	YMB char *pName = "compile_commands.json";
    /*
	 * FILE *pFd = fopen(pName, "w");
	 * if (!pFd)
	 * {
	 * 	PERROR_LOG("CompileCmdJson");
	 * 	return Y_ERROR_JSON;
	 * }
	 * fprintf(pFd, "%s", CCJSON_BEGIN);
     */
	printf("%s", CCJSON_BEGIN);
	YMB size_t totalSize = 0;
	YMB size_t count = 0;
	size_t i = 0;
	size_t lastOne = nbCmds - 1; 
	char *pEndComa = ",\n";
	while (i < nbCmds)
	{
		printf("[%zu]: %s\n", i, ppCmds[i]);
		
		/* printf("%s%s%s,", Y_OPENCURLY, Y_DIRECTORY, PUTINQUOTE(YCWD), Y_FILE, PUTINQUOTE(ppCmds[i])) */
		if (i == lastOne)
			pEndComa = "\n";
		i++;
	}
	printf("%s", CCJSON_END);
    /*
	 * fprintf(pFd, "%s", CCJSON_END);
	 * fclose(pFd);
     */
	return Y_SUCCESS;
}
