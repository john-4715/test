#ifndef _LIB_UTILS_H_
#define _LIB_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

	int convertIp(char *strIp, unsigned int addr);

	int getSubNet(char *strsubnet, unsigned int addr);

#ifdef __cplusplus
}
#endif

#endif