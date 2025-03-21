#include "utils.h"

int convertIp(char *strIp, unsigned int addr)
{
	if (NULL == strIp)
	{
		return -1;
	}
	sprintf(strIp, "%u.%u.%u.%u", ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2],
			((unsigned char *)&addr)[3]);
	return 0;
}

int getSubNet(char *strsubnet, unsigned int addr)
{
	if (NULL == strsubnet)
	{
		return -1;
	}
	sprintf(strsubnet, "%u.%u.%u", ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1],
			((unsigned char *)&addr)[2]);
	return 0;
}