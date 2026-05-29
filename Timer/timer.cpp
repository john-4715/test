
#include "timer.h"
#include <errno.h>
#include <stdlib.h>

// 线程函数实现
void *Timer::thread_func(void *arg)
{
	Timer *timer = static_cast<Timer *>(arg);

	struct timespec ts;
	int result;

	while (true)
	{
		pthread_mutex_lock(&timer->mutex);

		// 检查是否需要停止
		if (timer->stop_flag)
		{
			pthread_mutex_unlock(&timer->mutex);
			break;
		}

		// 计算下次超时时间
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timer->interval_ms / 1000;
		ts.tv_nsec += (timer->interval_ms % 1000) * 1000000;

		// 处理纳秒溢出
		if (ts.tv_nsec >= 1000000000)
		{
			ts.tv_sec++;
			ts.tv_nsec -= 1000000000;
		}

		// 等待超时或被唤醒
		result = pthread_cond_timedwait(&timer->cond, &timer->mutex, &ts);

		pthread_mutex_unlock(&timer->mutex);

		// 超时触发回调
		if (result == ETIMEDOUT)
		{
			if (timer->callback)
			{
				timer->callback(timer->user_data);
			}

			// 如果是单次定时器，则停止
			if (timer->single_shot)
			{
				pthread_mutex_lock(&timer->mutex);
				timer->state = TIMER_STOPPED;
				timer->stop_flag = true;
				pthread_mutex_unlock(&timer->mutex);
				break;
			}
		}
	}

	return NULL;
}

// 构造函数实现
Timer::Timer(int interval, TimerCallback cb, void *data, bool single)
	: state(TIMER_STOPPED), interval_ms(interval), single_shot(single), callback(cb), user_data(data), stop_flag(false)
{
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

// 析构函数实现
Timer::~Timer()
{
	stop();
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

// 启动定时器实现
int Timer::start()
{
	pthread_mutex_lock(&mutex);

	if (state == TIMER_RUNNING)
	{
		pthread_mutex_unlock(&mutex);
		return 0; // 已经在运行
	}

	stop_flag = false;
	state = TIMER_RUNNING;

	pthread_mutex_unlock(&mutex);

	// 创建定时器线程
	if (pthread_create(&thread, NULL, thread_func, this) != 0)
	{
		state = TIMER_STOPPED;
		return -1;
	}

	return 0;
}

// 停止定时器实现
int Timer::stop()
{
	pthread_mutex_lock(&mutex);

	if (state != TIMER_RUNNING)
	{
		pthread_mutex_unlock(&mutex);
		return 0; // 已经停止
	}

	stop_flag = true;
	state = TIMER_STOPPED;

	// 唤醒等待的线程
	pthread_cond_signal(&cond);

	pthread_mutex_unlock(&mutex);

	// 等待线程结束
	if (pthread_join(thread, NULL) != 0)
	{
		return -1;
	}

	return 0;
}

// 检查定时器是否活跃实现
bool Timer::is_active() const { return (state == TIMER_RUNNING); }
