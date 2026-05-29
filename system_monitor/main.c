#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// 定义统计参数
#define DURATION 360 // 6分钟 = 360秒
#define INTERVAL 1	 // 每秒执行一次
#define SAMPLE_COUNT DURATION

// 图片尺寸
#define IMG_WIDTH 1200
#define IMG_HEIGHT 600
#define MARGIN_LEFT 80
#define MARGIN_RIGHT 60
#define MARGIN_TOP 50
#define MARGIN_BOTTOM 80
#define GRAPH_WIDTH (IMG_WIDTH - MARGIN_LEFT - MARGIN_RIGHT)
#define GRAPH_HEIGHT (IMG_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM)

typedef struct
{
	double cpu_usage;
	double memory_usage;
	double timestamp;
} SampleData;

// 获取CPU使用率
double get_cpu_usage()
{
	static long long prev_idle = 0, prev_total = 0;
	long long idle, total;
	long long user, nice, system, idle_time, iowait, irq, softirq, steal;

	FILE *fp = fopen("/proc/stat", "r");
	if (!fp)
		return -1;

	fscanf(fp, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal);
	fclose(fp);

	idle = idle_time + iowait;
	total = user + nice + system + idle + iowait + irq + softirq + steal;

	double usage = 0;
	if (prev_total != 0)
	{
		long long total_diff = total - prev_total;
		long long idle_diff = idle - prev_idle;
		usage = (total_diff - idle_diff) * 100.0 / total_diff;
	}

	prev_idle = idle;
	prev_total = total;

	return usage;
}

// 获取内存使用率
double get_memory_usage()
{
	FILE *fp = fopen("/proc/meminfo", "r");
	if (!fp)
		return -1;

	long long total_mem = 0, free_mem = 0, buffers = 0, cached = 0;
	char line[256];

	while (fgets(line, sizeof(line), fp))
	{
		if (sscanf(line, "MemTotal: %lld kB", &total_mem) == 1)
			continue;
		if (sscanf(line, "MemFree: %lld kB", &free_mem) == 1)
			continue;
		if (sscanf(line, "Buffers: %lld kB", &buffers) == 1)
			continue;
		if (sscanf(line, "Cached: %lld kB", &cached) == 1)
			continue;
	}
	fclose(fp);

	if (total_mem == 0)
		return -1;

	long long used_mem = total_mem - free_mem - buffers - cached;
	return (used_mem * 100.0) / total_mem;
}

// 创建PPM文件（简单的图像格式）
void create_ppm(const char *filename, SampleData *samples, int count)
{
	FILE *fp = fopen(filename, "w");
	if (!fp)
	{
		printf("无法创建文件 %s\n", filename);
		return;
	}

	// PPM头部
	fprintf(fp, "P3\n%d %d\n255\n", IMG_WIDTH, IMG_HEIGHT);

	// 找到CPU和内存的最大最小值
	double max_cpu = 0, min_cpu = 100;
	double max_mem = 0, min_mem = 100;
	for (int i = 0; i < count; i++)
	{
		if (samples[i].cpu_usage > max_cpu)
			max_cpu = samples[i].cpu_usage;
		if (samples[i].cpu_usage < min_cpu)
			min_cpu = samples[i].cpu_usage;
		if (samples[i].memory_usage > max_mem)
			max_mem = samples[i].memory_usage;
		if (samples[i].memory_usage < min_mem)
			min_mem = samples[i].memory_usage;
	}

	// 添加一点边距
	max_cpu += 5;
	min_cpu = (min_cpu > 5) ? min_cpu - 5 : 0;
	max_mem += 5;
	min_mem = (min_mem > 5) ? min_mem - 5 : 0;

	for (int y = 0; y < IMG_HEIGHT; y++)
	{
		for (int x = 0; x < IMG_WIDTH; x++)
		{
			int r = 255, g = 255, b = 255; // 默认白色背景

			// 绘制边框
			if (x == MARGIN_LEFT - 1 || x == IMG_WIDTH - MARGIN_RIGHT || y == MARGIN_TOP - 1 || y == IMG_HEIGHT - MARGIN_BOTTOM)
			{
				r = g = b = 0; // 黑色边框
			}
			// 绘制网格线
			else if (x > MARGIN_LEFT && x < IMG_WIDTH - MARGIN_RIGHT && y > MARGIN_TOP && y < IMG_HEIGHT - MARGIN_BOTTOM)
			{
				// 水平网格线（每20%一条）
				for (int i = 0; i <= 5; i++)
				{
					int grid_y = MARGIN_TOP + (GRAPH_HEIGHT * i / 5);
					if (abs(y - grid_y) < 1)
					{
						r = g = b = 200; // 浅灰色网格线
						break;
					}
				}
				// 垂直网格线（每分钟一条）
				for (int i = 0; i <= 6; i++)
				{
					int grid_x = MARGIN_LEFT + (GRAPH_WIDTH * i / 6);
					if (abs(x - grid_x) < 1)
					{
						r = g = b = 200;
						break;
					}
				}

				// 绘制CPU曲线（红色）
				if (x >= MARGIN_LEFT && x <= MARGIN_LEFT + GRAPH_WIDTH)
				{
					int sample_idx = (x - MARGIN_LEFT) * count / GRAPH_WIDTH;
					if (sample_idx < count - 1)
					{
						double cpu_val = samples[sample_idx].cpu_usage;
						int cpu_y = MARGIN_TOP + GRAPH_HEIGHT - (cpu_val - min_cpu) * GRAPH_HEIGHT / (max_cpu - min_cpu);

						if (abs(y - cpu_y) < 2)
						{
							r = 255;
							g = 0;
							b = 0; // 红色
						}
					}
				}

				// 绘制内存曲线（蓝色）
				if (x >= MARGIN_LEFT && x <= MARGIN_LEFT + GRAPH_WIDTH)
				{
					int sample_idx = (x - MARGIN_LEFT) * count / GRAPH_WIDTH;
					if (sample_idx < count - 1)
					{
						double mem_val = samples[sample_idx].memory_usage;
						int mem_y = MARGIN_TOP + GRAPH_HEIGHT - (mem_val - min_mem) * GRAPH_HEIGHT / (max_mem - min_mem);

						if (abs(y - mem_y) < 2)
						{
							r = 0;
							g = 0;
							b = 255; // 蓝色
						}
					}
				}
			}
			// 绘制刻度标签区域
			else
			{
				// 左侧Y轴刻度标签
				if (x < MARGIN_LEFT && x > MARGIN_LEFT - 40)
				{
					for (int i = 0; i <= 5; i++)
					{
						int label_y = MARGIN_TOP + (GRAPH_HEIGHT * i / 5);
						if (abs(y - label_y) < 10)
						{
							r = g = b = 0;
							break;
						}
					}
				}
				// 底部X轴刻度标签
				if (y > IMG_HEIGHT - MARGIN_BOTTOM && y < IMG_HEIGHT - MARGIN_BOTTOM + 20)
				{
					for (int i = 0; i <= 6; i++)
					{
						int label_x = MARGIN_LEFT + (GRAPH_WIDTH * i / 6);
						if (abs(x - label_x) < 10)
						{
							r = g = b = 0;
							break;
						}
					}
				}
			}

			fprintf(fp, "%d %d %d ", r, g, b);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
	printf("图表已保存到 %s\n", filename);
}

// 简单的PNG转换（使用系统命令将PPM转换为PNG）
void convert_to_png(const char *ppm_file, const char *png_file)
{
	char cmd[256];
	sprintf(cmd, "convert %s %s 2>/dev/null", ppm_file, png_file);
	if (system(cmd) == 0)
	{
		printf("PNG图片已生成: %s\n", png_file);
		remove(ppm_file); // 删除临时PPM文件
	}
	else
	{
		printf("PNG转换失败，请安装ImageMagick: sudo apt-get install imagemagick\n");
		printf("临时文件保存在: %s\n", ppm_file);
	}
}

int main()
{
	SampleData samples[SAMPLE_COUNT];
	int sample_count = 0;

	printf("开始统计CPU和内存使用情况，持续6分钟...\n");
	printf("每秒采样一次，共 %d 个样本点\n", SAMPLE_COUNT);
	printf("按 Ctrl+C 可以提前结束\n\n");

	// 数据采集循环
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		samples[i].cpu_usage = get_cpu_usage();
		samples[i].memory_usage = get_memory_usage();
		samples[i].timestamp = i;
		sample_count++;

		// 显示进度
		int minutes = i / 60;
		int seconds = i % 60;
		printf("\r采样 %d/%d [%02d:%02d] CPU: %5.1f%% 内存: %5.1f%%", i + 1, SAMPLE_COUNT, minutes, seconds, samples[i].cpu_usage,
			   samples[i].memory_usage);
		fflush(stdout);

		sleep(INTERVAL);
	}

	printf("\n\n采样完成！正在生成图表...\n");

	// 保存原始数据到CSV文件
	FILE *csv = fopen("system_stats.csv", "w");
	if (csv)
	{
		fprintf(csv, "时间(秒),CPU使用率(%%),内存使用率(%%)\n");
		for (int i = 0; i < sample_count; i++)
		{
			fprintf(csv, "%d,%.2f,%.2f\n", i, samples[i].cpu_usage, samples[i].memory_usage);
		}
		fclose(csv);
		printf("原始数据已保存到 system_stats.csv\n");
	}

	// 创建PPM图像文件
	create_ppm("output.ppm", samples, sample_count);

	// 转换为PNG
	convert_to_png("output.ppm", "system_stats.png");

	// 计算统计信息
	double cpu_sum = 0, mem_sum = 0;
	double cpu_max = 0, mem_max = 0;
	double cpu_min = 100, mem_min = 100;

	for (int i = 0; i < sample_count; i++)
	{
		cpu_sum += samples[i].cpu_usage;
		mem_sum += samples[i].memory_usage;
		if (samples[i].cpu_usage > cpu_max)
			cpu_max = samples[i].cpu_usage;
		if (samples[i].cpu_usage < cpu_min)
			cpu_min = samples[i].cpu_usage;
		if (samples[i].memory_usage > mem_max)
			mem_max = samples[i].memory_usage;
		if (samples[i].memory_usage < mem_min)
			mem_min = samples[i].memory_usage;
	}

	printf("\n统计信息:\n");
	printf("CPU使用率 - 平均: %.2f%%, 最大: %.2f%%, 最小: %.2f%%\n", cpu_sum / sample_count, cpu_max, cpu_min);
	printf("内存使用率 - 平均: %.2f%%, 最大: %.2f%%, 最小: %.2f%%\n", mem_sum / sample_count, mem_max, mem_min);

	return 0;
}