#include <math.h>
#include <time.h>

#include "audio_data.h"
#include "clk.h"

#define EMIO_GPIO_DIR "/sys/class/gpio"
#define EMIO_PIN_BASE 426

int gpio_init(AUDIO_CHANNELS *pHandle, int pin_num)
{
	int fd;
	char buffer[64];
	int ret;

	sprintf(buffer, "%s/export", EMIO_GPIO_DIR);
	printf("%s\n", buffer);
	fd = open(buffer, O_WRONLY);
	if (fd < 0)
	{
		perror("open export for writing");
		return -1;
	}
	sprintf(buffer, "%d", EMIO_PIN_BASE + pin_num);
	printf("%s\n", buffer);
	ret = write(fd, buffer, sizeof(buffer));
	if (ret < 0)
	{
		perror(buffer);
		return -1;
	}
	close(fd);

	// 设置GPIO方向
	sprintf(buffer, "%s/gpio%d/direction", EMIO_GPIO_DIR, EMIO_PIN_BASE + pin_num);
	printf("%s\n", buffer);
	fd = open(buffer, O_RDWR);
	if (fd < 0)
	{
		perror("open direction for reading and writing");
		return -1;
	}
	sprintf(buffer, "%s", "out");
	printf("%s\n", buffer);

	ret = write(fd, buffer, 3); // 设置为输出
	if (ret < 0)
	{
		perror(buffer);
		return -1;
	}
	close(fd);
	return 0;
}

int write_gpio_value(AUDIO_CHANNELS *pHandle, int pin_num, int value)
{
	int fd;
	char buffer[64];
	int ret;
	// 写入EMIO引脚值
	sprintf(buffer, "%s/gpio%d/value", EMIO_GPIO_DIR, EMIO_PIN_BASE + pin_num);
	fd = open(buffer, O_RDWR);
	if (fd < 0)
	{
		perror("open value for reading and writing");
		return -1;
	}

	sprintf(buffer, "%d", value);
	ret = write(fd, buffer, 1);
	if (ret < 0)
	{
		perror(buffer);
		return -1;
	}
	close(fd);
	return 0;
}

int gpio_destroy(AUDIO_CHANNELS *pHandle, int pin_num)
{
	int fd;
	char buffer[64];
	int ret;
	// 取消EMIO引脚的导出
	sprintf(buffer, "%s/unexport", EMIO_GPIO_DIR);
	printf("%s\n", buffer);
	fd = open(buffer, O_WRONLY);
	if (fd < 0)
	{
		perror("open unexport for writing");
		return -1;
	}
	sprintf(buffer, "%d", EMIO_PIN_BASE + pin_num);
	printf("%s\n", buffer);
	ret = write(fd, buffer, sizeof(buffer));
	if (ret < 0)
	{
		perror(buffer);
		return -1;
	}
	close(fd);
	return 0;
}

void lrck_cycle_fun(void *arg)
{
	printf("enter lrck_cycle_fun.\n");
	AUDIO_CHANNELS *pHandle = (AUDIO_CHANNELS *)arg;

	int fd;
	char buffer[64];
	int ret;

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 40000; // 纳秒

	int pin_num = pHandle->audio_ch[pHandle->channel_idx].lrck_gpio;

	printf("lrck_cycle_fun:pin_num=%d\n", pin_num);

	gpio_init(pHandle, pin_num);

	while (1)
	{
		// 写入EMIO引脚值
		sprintf(buffer, "%s/gpio%d/value", EMIO_GPIO_DIR, EMIO_PIN_BASE + pin_num);
		fd = open(buffer, O_RDWR);
		if (fd < 0)
		{
			perror("open value for reading and writing");
			break;
		}

		// 输出低电平
		sprintf(buffer, "%s", "0");
		ret = write(fd, buffer, 1);
		if (ret < 0)
		{
			perror(buffer);
			break;
		}

		// 发送signal
		// pthread_mutex_lock(&pHandle->sclk_rmutex);
		// pthread_cond_signal(&pHandle->sclk_cond);
		// pthread_mutex_unlock(&pHandle->sclk_rmutex);
		// printf("send sclk_cond...\n");

		nanosleep(&ts, NULL);
		// 输出高电平
		sprintf(buffer, "%s", "1");
		ret = write(fd, buffer, 1);
		if (ret < 0)
		{
			perror(buffer);
			break;
		}

		close(fd);
		nanosleep(&ts, NULL);
	}
	gpio_destroy(pHandle, pin_num);
}

void sclk_cycle_fun(void *arg)
{
	AUDIO_CHANNELS *pHandle = (AUDIO_CHANNELS *)arg;

	int fd;
	char buffer[64];
	int ret;

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 122; // 纳秒
	int pin_num = pHandle->audio_ch[pHandle->channel_idx].sclk_gpio;
	int stdin_gpio_pin = pHandle->audio_ch[pHandle->channel_idx].stdin_gpio;

	printf("sclk_cycle_fun:pin_num=%d\n", pin_num);
	printf("stdin_gpio_pin=%d\n", stdin_gpio_pin);

	gpio_init(pHandle, pin_num);
	gpio_init(pHandle, stdin_gpio_pin);

	int flag = false;
	static int xIdx = 0;

	while (1)
	{
		// int ft = 10;		  /* 周期 hz */
		// int fs = (64 * 1024); /* 采样频率*/
		// unsigned short input_data = 0;
		// int offset;
		// unsigned bit_var;

		pthread_cond_wait(&pHandle->sclk_cond, &pHandle->sclk_rmutex);
		printf("recv sclk_cond...\n");
		for (int i = 0; i < 64; i++)
		{
			// offset = xIdx % 16;
			// if (offset == 0)
			// {
			// 	input_data = 2 * cos(2 * 3.1415926 * ft * xIdx / fs) * 1000;
			// }
			// 写入EMIO引脚值
			sprintf(buffer, "%s/gpio%d/value", EMIO_GPIO_DIR, EMIO_PIN_BASE + pin_num);
			fd = open(buffer, O_RDWR);
			if (fd < 0)
			{
				perror("open value for reading and writing");
				break;
			}

			// 输出低电平
			sprintf(buffer, "%s", "0");
			ret = write(fd, buffer, 1);
			if (ret < 0)
			{
				perror(buffer);
				break;
			}

			// 发送signal
#if 0
			pthread_mutex_lock(&pHandle->mclk_rmutex);
			pthread_cond_signal(&pHandle->mclk_cond);
			pthread_mutex_unlock(&pHandle->mclk_rmutex);
			// printf("send mclk_cond...\n");
#endif

			// if (flag)
			// {
			// 	bit_var = (input_data >> (15 - offset) & 0x01);
			// 	write_gpio_value(pHandle, stdin_gpio_pin, bit_var);
			// }

			nanosleep(&ts, NULL);
			sprintf(buffer, "%s", "1");
			ret = write(fd, buffer, 1);
			if (ret < 0)
			{
				perror(buffer);
				break;
			}

			close(fd);
			nanosleep(&ts, NULL);
			if (flag == false)
			{
				flag = true;
			}
			xIdx++;
		}
		gpio_destroy(pHandle, pin_num);
		gpio_destroy(pHandle, stdin_gpio_pin);
	}
}

void mclk_cycle_fun(void *arg)
{
	AUDIO_CHANNELS *pHandle = (AUDIO_CHANNELS *)arg;

	int fd;
	char buffer[64];
	int ret;

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 7;

	int pin_num = pHandle->audio_ch[pHandle->channel_idx].mclk_gpio;
	printf("mclk_cycle_fun:pin_num=%d\n", pin_num);

	gpio_init(pHandle, pin_num);

	while (1)
	{
		pthread_cond_wait(&pHandle->mclk_cond, &pHandle->mclk_rmutex);
		printf("recv mclk_cond...\n");
		for (int i = 0; i < 16; i++)
		{
			// 写入EMIO引脚值
			sprintf(buffer, "%s/gpio%d/value", EMIO_GPIO_DIR, EMIO_PIN_BASE + pin_num);
			fd = open(buffer, O_RDWR);
			if (fd < 0)
			{
				perror("open value for reading and writing");
				break;
			}

			// 输出低电平
			sprintf(buffer, "%s", "0");
			ret = write(fd, buffer, 1);
			if (ret < 0)
			{
				perror(buffer);
				break;
			}

			nanosleep(&ts, NULL); // 延时7ns

			// 输出高电平
			sprintf(buffer, "%s", "1");
			ret = write(fd, buffer, 1);
			if (ret < 0)
			{
				perror(buffer);
				break;
			}

			close(fd);
			nanosleep(&ts, NULL); // 延时7ns
		}
	}
	gpio_destroy(pHandle, pin_num);
}