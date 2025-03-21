#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>

#define COMM_SERVER_ID 0x8000
#define COMM_CLIENT_ID 0x7e7e

// 请求消息类型
typedef enum
{
	REQ_CMD_TYPE_CPU,
	REQ_CMD_TYPE_MEM,
	REQ_CMD_TYPE_DISK,
	REQ_CMD_TYPE_NET
} BMC_REQ_CMD_TYPE;

// 上报消息类型
typedef enum
{
	BMC_RESP_CMD_PERIOD,
	BMC_RESP_CMD_CPU,
	BMC_RESP_CMD_MEM,
	BMC_RESP_CMD_DISK,
	BMC_RESP_CMD_NET
} BMC_RESP_CMD_TYPE;

typedef struct _BMC_MSG_HEADERE
{
	int32_t frameId;
	unsigned short srcId;
	unsigned short dstId;
} BMC_MSG_HEADERE;

typedef struct _BMC_MSG_BASE_INFO
{
	unsigned short cpu_stat;
	unsigned short cpu_num;
	int32_t cpu_temperature;
	int32_t cpu_used;  // cpu使用率
	int32_t mem_used;  // 内存使用率
	int32_t disk_used; // 磁盘使用率
} BMC_MSG_BASE_INFO;

// cpu信息查询结果
typedef struct _BMC_CPU_INFO
{
	unsigned short cpu_stat;
	unsigned short cpu_num;
	int32_t cpu_temperature;
	int32_t cpu_used; // cpu使用率
} BMC_CPU_INFO;

// 内存信息查询结果
typedef struct _BMC_MEM_INFO
{
	int32_t mem_used; // 内存使用率
} BMC_MEM_INFO;

// 磁盘信息查询结果
typedef struct _BMC_DISK_INFO
{
	int32_t disk_used; // 磁盘使用率
} BMC_DISK_INFO;

// 主动上报,使用率需进行转换，传送时乘1000,使用时要除以1000
typedef struct _PERRIOD_REPORT_s
{
	BMC_MSG_HEADERE header;
	unsigned short length;
	unsigned short reportType; // 上报消息类型
	BMC_MSG_BASE_INFO base_info;
	unsigned int crc;
} PERRIOD_REPORT_t;

// bmc查询请求
typedef struct _BMC_REQUEST_s
{
	BMC_MSG_HEADERE header;
	unsigned short length;
	unsigned short cmdType; // 请求类型
	unsigned int crc;
} BMC_REQUEST_t;

typedef struct _BMC_RESP_INFO
{
	BMC_MSG_HEADERE header;
	unsigned short length;
	unsigned short reportType; // 上报消息类型
	union
	{
		BMC_CPU_INFO cpu_info;
		BMC_MEM_INFO mem_info;
		BMC_DISK_INFO dsk_info;
	};
	unsigned int crc;
} BMC_RESP_INFO;

#endif