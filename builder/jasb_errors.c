#ifndef JASB_ERRORS_C
#define JASB_ERRORS_C

#include "jasb_errors.h"

char *pErrorMsg[] = {
	"No error.",
	"List is empty.",
	"Unknown error.",
	"Execution error.",
	"Build couldn't finish.",
	"Link couldn't finish.",
	"Json couldn't not be created.",
	"Thread error.",
	"Error while joining threads.",
};

char*
GetErrorMsg(int error)
{
	return pErrorMsg[error];
}

#ifdef _WIN32

#elif __linux__

#endif
#endif // JASB_ERRORS_C
