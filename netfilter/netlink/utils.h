#ifndef __UTILS_H__
#define __UTILS_H__

#include <linux/string.h>

int convertIp(char *strIp, unsigned int addr);

int getSubNet(char *strsubnet, unsigned int addr);

#endif