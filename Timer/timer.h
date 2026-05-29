
#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <time.h>

// 定时器状态枚举
typedef enum
{
	TIMER_STOPPED,
	TIMER_RUNNING,
	TIMER_PAUSED
} TimerState;

// 定时器回调函数类型
typedef void (*TimerCallback)(void *user_data);

// 定时器类
class Timer
{
private:
	TimerState state;
	int interval_ms;
	bool single_shot;
	TimerCallback callback;
	void *user_data;
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool stop_flag;

	// 线程函数
	static void *thread_func(void *arg);

public:
	// 构造函数
	Timer(int interval, TimerCallback cb, void *data = NULL, bool single = false);

	// 析构函数
	~Timer();

	// 启动定时器
	int start();

	// 停止定时器
	int stop();

	// 检查定时器是否活跃
	bool is_active() const;
};

#endif // TIMER_H
