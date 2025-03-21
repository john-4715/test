#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define READMAX 1024

char *sf_strdup(const char *s);

char *sfget_shell(char *cmd);

void sf_free(void *p);

unsigned long get_file_size(const char *path);

void delete_file(char *filepath);

#endif