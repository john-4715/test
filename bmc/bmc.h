#ifndef __BMC_H__
#define __BMC_H__

#include <pthread.h>

#include "linklist.h"

typedef struct _BMC_HANDLE_s
{
	int fd;
	LinkList *sendlist;
	pthread_mutex_t sendlist_mutex;
	pthread_mutex_t send_cond_lock;
	pthread_cond_t send_cond;
} BMC_HANDLE_t;

#endif