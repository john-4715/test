#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define HOST_NAME_MAX 64
#define DOMAIN_NAME_MAX 64

const char *HOST_NAME_FILE = "/etc/hostname";
const char *DOMAIN_NAME_FILE = "/etc/domainname";

int getHostNameConfig(char *hostname)
{
	int ret = 0;
	strcpy(hostname, ""); // reset it to empty at first.

	FILE *fp = fopen(HOST_NAME_FILE, "r");
	if (fp == NULL)
	{
		return -1;
	}

	// fgets(): reads in at most one less than size characters from stream and stores them into the buffer pointed to by
	// s.
	fgets(hostname, HOST_NAME_MAX + 1, fp);

	if (NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}
	return ret;
}

int getDomainNameConfig(char *name)
{
	int ret = 0;
	strcpy(name, ""); // reset it to empty at first.

	FILE *fp = fopen(DOMAIN_NAME_FILE, "r");
	if (fp == NULL)
	{
		return -1;
	}

	// fgets(): reads in at most one less than size characters from stream and stores them into the buffer pointed to by
	// s.
	fgets(name, DOMAIN_NAME_MAX + 1, fp);

	if (NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return ret;
}

int main()
{
    getHostNameConfig();
    getDomainNameConfig();
    printf("domain name is :\n");
    return 0;
}