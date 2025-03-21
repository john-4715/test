#include <dirent.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bmc.h"
#include "protocol.h"
#include "sys_info.h"
#include "usart.h"
#include "utils.h"

unsigned int crc = 0xffffffff;

int get_sys_version(struct utsname *sysinfo)
{
	if (uname(sysinfo) == -1)
	{
		perror("uname");
		return 1;
	}
	printf("System name: %s\n", sysinfo->sysname);
	printf("Node name: %s\n", sysinfo->nodename);
	printf("Release: %s\n", sysinfo->release);
	printf("Version: %s\n", sysinfo->version);
	printf("Machine: %s\n", sysinfo->machine);
	return 0;
}

int get_cpu_status(unsigned short *status)
{
	FILE *fp;
	char buffer[1024];
	// int is_faulty = 0;
	long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;

	fp = fopen("/proc/stat", "r");
	if (fp == NULL)
	{
		perror("打开文件/proc/stat失败");
		return -1;
	}

	// 读取并解析CPU时间段的行
	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		// 解析CPU时间段，假设第一个是cpu行
		if (sscanf(buffer, "cpu %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle, &iowait, &irq,
				   &softirq, &steal, &guest, &guest_nice) >= 10)
		{
			// 检查空闲时间是否在递增，这是CPU正常工作的一个简单指标
			if (idle >= 0 && iowait >= 0)
			{
				// printf("CPU工作状态看起来是正常的。\n");
				*status = 1;
			}
			else
			{
				// printf("CPU工作状态异常。\n");
				*status = 0;
			}

			if (strstr(buffer, "cpu errors") != NULL)
			{
				// printf("CPU可能存在故障。\n");
				*status = 2;
				break;
			}
		}
	}

	fclose(fp);
	return 0;
}

float cal_occupy(CPU_OCCUPY *o, CPU_OCCUPY *n)
{
	float cpu_used;
	double od, nd;
	double id, sd;

	od = (double)(o->user + o->nice + o->system + o->idle); // 第一次(用户+优先级+系统+空闲)的时间再赋给od
	nd = (double)(n->user + n->nice + n->system + n->idle); // 第二次(用户+优先级+系统+空闲)的时间再赋给od
	// float scale = 100.0 / (float)(nd - od); // 100除强制转换(nd-od)之差为float类型再赋给scale这个变量
	id = (double)(n->user - o->user);	  // 用户第一次和第二次的时间之差再赋给id
	sd = (double)(n->system - o->system); // 系统第一次和第二次的时间之差再赋给sd
	cpu_used = ((sd + id) * 100.0) / (nd - od); //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给cpu_used
	return cpu_used;
}

void get_occupy(CPU_OCCUPY *o)
{
	FILE *fd;

	char buff[MAXBUFSIZE];
	// 这里只读取stat文件的第一行及cpu总信息，如需获取每核cpu的使用情况，请分析stat文件的接下来几行。
	fd = fopen("/proc/stat", "r");
	fgets(buff, sizeof(buff), fd);
	sscanf(buff, "%s %u %u %u %u", o->name, &o->user, &o->nice, &o->system, &o->idle);
	fclose(fd);
}

int get_mem_occupy(float *memory_usage)
{
	FILE *fd;
	// char buff[MAXBUFSIZE];
	fd = fopen("/proc/meminfo", "r");
	if (fd == NULL)
	{
		perror("fopen");
		return 0;
	}

	char line[256];
	unsigned long total, free, used, buffers, cached;
	total = free = used = buffers = cached = 0;

	// 解析文件中的内存信息
	while (fgets(line, sizeof(line), fd) != NULL)
	{
		if (sscanf(line, "MemTotal: %lu kB\n", &total) == 1)
		{
			// 读取总内存
		}
		else if (sscanf(line, "MemFree: %lu kB\n", &free) == 1)
		{
			// 读取空闲内存
		}
		else if (sscanf(line, "Buffers: %lu kB\n", &buffers) == 1)
		{
			// 读取缓冲内存
		}
		else if (sscanf(line, "Cached: %lu kB\n", &cached) == 1)
		{
			// 读取缓存内存
		}
	}

	fclose(fd);
	used = total - free - buffers - cached;

	// 计算占用率，并打印
	*memory_usage = (float)used / total * 100;
	printf("Memory Usage: %.2f%%\n", *memory_usage);
	return 1;
}

int get_cpu_temperature(CPU_TEMP_ARRAY *temp_arry, int *corenum)
{

	char tempfilename[2][32] = {"temp1_input", "temp2_input"};
	for (int i = 0; i < 2; i++)
	{
		FILE *thermal_file;
		char path[256];
		sprintf(path, "/sys/class/hwmon/hwmon0/device/hwmon/hwmon0/%s", tempfilename[i]);
		// 假设我们只关心第一个核（CPU0）的温度
		thermal_file = fopen(path, "r");
		if (thermal_file == NULL)
		{
			perror("Error opening thermal file");
			return -1;
		}

		// 读取温度值，它以微秒为单位
		if (fscanf(thermal_file, "%d", &temp_arry->temperature[i]) != 1)
		{
			fclose(thermal_file);
			perror("Error reading temperature");
			return -1;
		}
		fclose(thermal_file);

		// 将温度从微秒转换为摄氏度，并打印
		printf("CPU Temperature: %.2f°C\n", temp_arry->temperature[i] / 1000.0);
	}
	*corenum = 2;

	return 0;
}

int get_cpu_frequency(CPU_FREQ_ARRAY *freq_arry, int *corenum)
{
	char buf[100] = {0};
	int i = 0;
	// 读取CPU信息，例如使用率
	FILE *cpuinfo_file = fopen("/proc/cpuinfo", "r");
	if (cpuinfo_file == NULL)
	{
		perror("Error opening cpuinfo file");
		return -1;
	}
	char strfreq[32] = {0};
	// 查找“cpu MHz”字段
	while (fgets(buf, sizeof(buf), cpuinfo_file))
	{
		if (strncmp(buf, "cpu MHz", 7) == 0)
		{
			printf("CPU MHz: %s", strchr(buf, ':') + 1);
			strcpy(strfreq, strchr(buf, ':') + 1);
			freq_arry->frequency[i] = atoi(strfreq);
			i++;
		}
	}
	*corenum = i;
	fclose(cpuinfo_file);
	return 0;
}

float get_io_occupy()
{
	char cmd[] = "iostat -d -x";
	char buffer[MAXBUFSIZE];
	char a[20];
	float arr[20];
	FILE *pipe = popen(cmd, "r");
	if (!pipe)
		return -1;
	fgets(buffer, sizeof(buffer), pipe);
	fgets(buffer, sizeof(buffer), pipe);
	fgets(buffer, sizeof(buffer), pipe);
	fgets(buffer, sizeof(buffer), pipe);
	sscanf(buffer, "%s %f %f %f %f %f %f %f %f %f %f %f %f %f ", a, &arr[0], &arr[1], &arr[2], &arr[3], &arr[4],
		   &arr[5], &arr[6], &arr[7], &arr[8], &arr[9], &arr[10], &arr[11], &arr[12]);
	// printf("%f\n",arr[12]);
	return arr[12];
	pclose(pipe);
}

void get_disk_occupy(char *reused)
{
	char currentDirectoryPath[MAXBUFSIZE];
	getcwd(currentDirectoryPath, MAXBUFSIZE);
	// printf("当前目录：%s\n",currentDirectoryPath);
	char cmd[50] = "df ";
	strcat(cmd, currentDirectoryPath);
	// printf("%s\n",cmd);

	char buffer[MAXBUFSIZE];
	FILE *pipe = popen(cmd, "r");
	char fileSys[20];
	char blocks[20];
	char used[20];
	char free[20];
	char percent[10];
	char moment[20];

	if (!pipe)
		return;
	if (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		sscanf(buffer, "%s %s %s %s %s %s", fileSys, blocks, used, free, percent, moment);
	}
	if (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		sscanf(buffer, "%s %s %s %s %s %s", fileSys, blocks, used, free, percent, moment);
	}
	// printf("desk used:%s\n",percent);
	strcpy(reused, percent);
	return;
}

int getCurrentDownloadRates(long int *save_rate)
{
	char intface[64] = {0}; // 这是网络接口名，根据主机配置
	getNetcardName(intface);

	FILE *net_dev_file;
	char buffer[1024];
	// size_t bytes_read;
	// char *match;
	if ((net_dev_file = fopen("/proc/net/dev", "r")) == NULL)
	{
		printf("open file /proc/net/dev/ error!\n");
		return -1;
	}

	int i = 0;
	while (i++ < 20)
	{
		if (fgets(buffer, sizeof(buffer), net_dev_file) != NULL)
		{
			if (strstr(buffer, intface) != NULL)
			{
				// printf("%d   %s\n",i,buffer);
				sscanf(buffer, "%s %ld", buffer, save_rate);
				break;
			}
		}
	}
	if (i == 20)
		*save_rate = 0.01;
	fclose(net_dev_file); // 关闭文件
	return 0;
}

int getNetcardName(char *netcardName)
{
	DIR *dir = opendir("/sys/class/net");
	if (dir == NULL)
	{
		perror("opendir");
		return -1;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && !(strstr(entry->d_name, "lo")) &&
			!(strstr(entry->d_name, "virbr0")))
		{
			printf("网卡名称: %s\n", entry->d_name);
			sprintf(netcardName, "%s", entry->d_name);
			break;
		}
	}

	closedir(dir);
	return 0;
}

int get_mac_address(char *net_name, char *macAddr)
{
	struct ifreq ifreq;
	int sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket ");
		return 2;
	}
	strcpy(ifreq.ifr_name, net_name);
	if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
	{
		perror("ioctl ");
		return 3;
	}
	sprintf(macAddr, "%02x:%02x:%02x:%02x:%02x:%02x\n ", (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[1], (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[3], (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
	return 0;
}

int get_ip_address(const char *net_name, char *ip_addr)
{
	if (ip_addr == NULL)
	{
		return -1;
	}
	struct ifreq s;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	// int result;

	strcpy(s.ifr_name, net_name);
	if (ioctl(fd, SIOCGIFFLAGS, &s) == 0)
	{
		// 确认网卡活动中
		if (s.ifr_flags & IFF_UP)
		{
			// printf("net running \n");
			if (ioctl(fd, SIOCGIFADDR, &s) == 0)
			{
				memcpy(ip_addr, &((struct sockaddr_in *)&s.ifr_addr)->sin_addr, 4);
			}
		}
	}

	return 0;
}

int get_netmask_address(const char *net_name, char *netmask_addr)
{
	if (netmask_addr == NULL)
	{
		return -1;
	}
	struct ifreq s;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	strcpy(s.ifr_name, net_name);
	if (ioctl(fd, SIOCGIFFLAGS, &s) == 0)
	{
		// 以下是一些其他的网卡信息能力获取接口
		// if(s.ifr_flags & IFF_UP) *if_flag |= IFSTAT_UP;
		// if(s.ifr_flags & IFF_BROADCAST) *if_flag |= IFSTAT_BROADCAST;
		// if(s.ifr_flags & IFF_LOOPBACK) *if_flag |= IFSTAT_LOOPBACK;
		// if(s.ifr_flags & IFF_POINTOPOINT) *if_flag |= IFSTAT_POINTOPOINT;
		// if(s.ifr_flags & IFF_MULTICAST) *if_flag |= IFSTAT_MULTICAST;

		if (s.ifr_flags & IFF_UP)
		{ // running
			// printf("net running \n");
			if (ioctl(fd, SIOCGIFNETMASK, &s) == 0)
			{
				memcpy(netmask_addr, &((struct sockaddr_in *)&s.ifr_addr)->sin_addr, 4);
			}
		}
	}

	return 0;
}

void report_timer_func(void *args)
{
	BMC_HANDLE_t *pHandle = (BMC_HANDLE_t *)args;

	while (1)
	{
		CPU_OCCUPY ocpu, ncpu;
		PERRIOD_REPORT_t reportBody = {0};
		unsigned char *pMsg = (unsigned char *)&reportBody;

		reportBody.header.srcId = COMM_SERVER_ID;
		reportBody.header.dstId = COMM_CLIENT_ID;
		int size = 0;
		reportBody.reportType = BMC_RESP_CMD_PERIOD;
		size += sizeof(unsigned short);

		struct utsname sysinfo;
		get_sys_version(&sysinfo);

		// 获取cpu状态
		get_cpu_status(&reportBody.base_info.cpu_stat);
		printf("cpu status = %d \n", reportBody.base_info.cpu_stat);

		// 获取cpu核数
		int cpu_num = sysconf(_SC_NPROCESSORS_ONLN);

		reportBody.base_info.cpu_num = cpu_num;

		// 获取cpu使用率
		get_occupy(&ocpu);
		sleep(1);
		get_occupy(&ncpu);
		float cpu_used = cal_occupy(&ocpu, &ncpu);
		printf("cpu used:%4.2f%%\n", cpu_used);
		reportBody.base_info.cpu_used = (uint32_t)(cpu_used * 1000);

		CPU_TEMP_ARRAY temp_arry = {0};
		get_cpu_temperature(&temp_arry, &cpu_num);
		reportBody.base_info.cpu_temperature = (uint32_t)temp_arry.temperature[0];

		// 获取内存使用率
		float mem_usage = 0.0;
		get_mem_occupy(&mem_usage);
		printf("mem used:%4.2f\n", mem_usage);
		reportBody.base_info.mem_used = (uint32_t)(mem_usage * 1000);

		// 获取io使用率
		// sprintf(send_buf, "io used:%4.2f,", get_io_occupy());
		// reportBody.mem_used = (uint32_t) using * 1000;

		// 获取当前磁盘的使用率
		char used[20] = {0};
		get_disk_occupy(used);
		printf("disk used:%s\n", used);
		reportBody.base_info.disk_used = (uint32_t)(atof(used) * 1000);

		// 网络延迟
		long int start_download_rates;					// 保存开始时的流量计数
		long int end_download_rates;					// 保存结果时的流量计数
		getCurrentDownloadRates(&start_download_rates); // 获取当前流量，并保存在start_download_rates里
		sleep(WAIT_SECOND); // 休眠多少秒，这个值根据宏定义中的WAIT_SECOND的值来确定
		getCurrentDownloadRates(&end_download_rates); // 获取当前流量，并保存在end_download_rates里
		printf("download is : %4.2f byte/s \n", ((float)(end_download_rates - start_download_rates)) / WAIT_SECOND);

		size += sizeof(BMC_MSG_BASE_INFO);
		reportBody.length = size + sizeof(unsigned int);
		reportBody.crc = crc32_cal(crc, pMsg, size);

		pthread_mutex_lock(&pHandle->sendlist_mutex);
		Linklist_Insert(pHandle->sendlist, (char *)&reportBody, sizeof(PERRIOD_REPORT_t));
		pthread_mutex_unlock(&pHandle->sendlist_mutex);

		pthread_mutex_lock(&pHandle->send_cond_lock);
		pthread_cond_signal(&pHandle->send_cond);
		pthread_mutex_unlock(&pHandle->send_cond_lock);
		// sleep(5);
	}
}

int sysInfoService(BMC_HANDLE_t *pHandle, char *buffer, int nlen)
{
	printf("receive data:\n");
	for (int i = 0; i < nlen; i++)
	{
		printf("%02x ", buffer[i]);
		if (i > 0 && i % 10 == 0)
		{
			printf("\n");
		}
	}
	printf("\n");

	char *psend = NULL;
	int sendlen = 0;
	BMC_REQUEST_t *pReq = (BMC_REQUEST_t *)buffer;
	if (pReq->header.srcId != COMM_CLIENT_ID || pReq->header.dstId != COMM_SERVER_ID)
	{
		printf("invalid memssage!\n");
		return -1;
	}
	int crcval = crc32_cal(crc, (unsigned char *)buffer, nlen - sizeof(unsigned int));

	if (crcval != pReq->crc)
	{
		printf("crc checksum fail, dropped!\n");
		return -1;
	}
	int cmdType = (int)pReq->cmdType;

	BMC_RESP_INFO bmc_resp_info = {0};
	bmc_resp_info.header.srcId = COMM_SERVER_ID;
	bmc_resp_info.header.dstId = COMM_CLIENT_ID;
	sendlen = sizeof(BMC_MSG_HEADERE);
	bmc_resp_info.length = 0;
	switch (cmdType)
	{
	case REQ_CMD_TYPE_CPU:
	{
		bmc_resp_info.reportType = BMC_RESP_CMD_CPU;
		sendlen += sizeof(unsigned short) * 2;
		bmc_resp_info.length += sizeof(unsigned short);
		/* ------获取cpu信息------*/
		get_cpu_status(&bmc_resp_info.cpu_info.cpu_stat);
		int cpu_num = sysconf(_SC_NPROCESSORS_ONLN);
		bmc_resp_info.cpu_info.cpu_num = cpu_num;
		CPU_OCCUPY ocpu, ncpu;
		get_occupy(&ocpu);
		// sleep(1);
		usleep(100 * 1000);
		get_occupy(&ncpu);
		float cpu_used = cal_occupy(&ocpu, &ncpu);
		bmc_resp_info.cpu_info.cpu_used = (uint32_t)(cpu_used * 1000);
		CPU_TEMP_ARRAY temp_arry = {0};
		get_cpu_temperature(&temp_arry, &cpu_num);
		bmc_resp_info.cpu_info.cpu_temperature = (uint32_t)temp_arry.temperature[0];
		sendlen += sizeof(BMC_CPU_INFO);
		bmc_resp_info.length += sizeof(BMC_CPU_INFO);
		bmc_resp_info.length += sizeof(unsigned int);
		/* --------------------*/
		bmc_resp_info.crc = crc32_cal(crc, (unsigned char *)&bmc_resp_info, sendlen);
		sendlen += sizeof(unsigned int);

		psend = (char *)&bmc_resp_info;
		break;
	}
	case REQ_CMD_TYPE_MEM:
	{
		bmc_resp_info.reportType = BMC_RESP_CMD_MEM;
		sendlen += sizeof(unsigned short) * 2;
		bmc_resp_info.length += sizeof(unsigned short);
		/*-------获取内存信息--------*/
		float mem_usage = 0.0;
		get_mem_occupy(&mem_usage);
		bmc_resp_info.mem_info.mem_used = (uint32_t)(mem_usage * 1000);

		sendlen += sizeof(BMC_MEM_INFO);
		bmc_resp_info.length += sizeof(BMC_MEM_INFO);
		bmc_resp_info.length += sizeof(unsigned int);
		/*-------------------------*/
		bmc_resp_info.crc = crc32_cal(crc, (unsigned char *)&bmc_resp_info, sendlen);
		sendlen += sizeof(unsigned int);

		psend = (char *)&bmc_resp_info;
		break;
	}
	case REQ_CMD_TYPE_DISK:
	{
		bmc_resp_info.reportType = BMC_RESP_CMD_MEM;
		sendlen += sizeof(unsigned short) * 2;
		bmc_resp_info.length += sizeof(unsigned short);
		/*-------获取磁盘信息--------*/
		char used[20] = {0};
		get_disk_occupy(used);
		bmc_resp_info.dsk_info.disk_used = (uint32_t)(atof(used) * 1000);
		sendlen += sizeof(BMC_DISK_INFO);
		bmc_resp_info.length += sizeof(BMC_DISK_INFO);
		bmc_resp_info.length += sizeof(unsigned int);
		/*-------------------------*/

		bmc_resp_info.crc = crc32_cal(crc, (unsigned char *)&bmc_resp_info, sendlen);
		sendlen += sizeof(unsigned int);

		break;
	}
	case REQ_CMD_TYPE_NET:
	{
		break;
	}
	default:
	{
		printf("invalid request.\n");
		break;
	}
	}

	if (sendlen <= 0)
	{
		printf("send message is empty!\n");
		return -1;
	}

	pthread_mutex_lock(&pHandle->sendlist_mutex);
	Linklist_Insert(pHandle->sendlist, (char *)psend, sendlen);
	pthread_mutex_unlock(&pHandle->sendlist_mutex);

	pthread_mutex_lock(&pHandle->send_cond_lock);
	pthread_cond_signal(&pHandle->send_cond);
	pthread_mutex_unlock(&pHandle->send_cond_lock);

	return 0;
}

void receive_func(void *args)
{
	BMC_HANDLE_t *pHandle = (BMC_HANDLE_t *)args;
	int len;
	char rcv_buf[1024];
	int offset = 0;
	char buffer[1024] = {0};
	int remainlen = 0;
	BMC_REQUEST_t *Req;

	while (1)
	{
		memset(rcv_buf, 0, 1024);
		len = UART0_Recv(pHandle->fd, rcv_buf, sizeof(rcv_buf));
		if (len > 0)
		{
			if (offset == 0 && len < sizeof(BMC_MSG_HEADERE) + 2) // 第一组数据长度不足消息头+2
			{
				memcpy(buffer + offset, rcv_buf, len);
				offset += len;
				continue;
			}
			if (remainlen == 0 && offset + len < sizeof(BMC_MSG_HEADERE) + 2)
			{
				memcpy(buffer + offset, rcv_buf, len);
				offset += len;
				continue;
			}
			// 此时获取后续数据长度
			if (remainlen == 0)
			{
				memcpy(buffer + offset, rcv_buf, len);
				Req = (BMC_REQUEST_t *)buffer;
				remainlen = Req->length;
			}

			if (offset + len < remainlen + sizeof(BMC_MSG_HEADERE) + 2)
			{
				memcpy(buffer + offset, rcv_buf, len);
				offset += len;
				continue;
			}
			else if (offset + len == remainlen + sizeof(BMC_MSG_HEADERE) + 2)
			{
				memcpy(buffer + offset, rcv_buf, len);
				offset += len;

				sysInfoService(pHandle, buffer, offset);
				offset = 0;
				remainlen = 0;
			}
			else
			{
				int tmplen = offset + len - remainlen + sizeof(BMC_MSG_HEADERE) + 2;

				memcpy(buffer + offset, rcv_buf, remainlen + sizeof(BMC_MSG_HEADERE) + 2);
				offset = remainlen + sizeof(BMC_MSG_HEADERE) + 2;
				sysInfoService(pHandle, buffer, offset);

				offset = tmplen;
				remainlen = 0;
			}
		}
		else
		{
			printf("cannot receive data\n");
		}
	}
}