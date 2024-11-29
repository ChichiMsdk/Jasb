#ifndef JASB_ERRORS_H
#define JASB_ERRORS_H

typedef enum yError {
	Y_SUCCESS = 0x00,

	Y_ERROR_EMPTY		= 0x01,
	Y_ERROR_UNKNOWN		= 0x02,
	Y_ERROR_EXEC		= 0x03,
	Y_ERROR_BUILD		= 0x04,
	Y_ERROR_LINK		= 0x05,
	Y_ERROR_JSON		= 0x06,
	Y_ERROR_THREAD		= 0x07,
	Y_ERROR_JOIN_THREAD	= 0x08,
	Y_ERROR_CLEANING	= 0x08,
	Y_MAX_ERROR
}yError;

extern char *pErrorMsg[];

#define JASB_CHECK(expr) \
	do { \
		yError result = Y_SUCCESS; \
		result = expr; \
		if (result != Y_SUCCESS) { \
			fprintf(stderr, "\"%s:%d\"[%d]: %s\n", __FILE__, __LINE__, result, GetErrorMsg(result));\
			return result; \
		} \
	} while(0);

char*	GetErrorMsg(int error);

#endif //JASB_ERRORS_H
