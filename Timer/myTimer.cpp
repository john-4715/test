
#include "myTimer.hpp"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct timerAttr_
{
	void (*callback)(void *);
	void *user_data;
} timerAttr;

// 信号处理函数
static void timer_signal_handler(union sigval sv)
{
	timerAttr *myTimerAttr= (timerAttr *)sv.sival_ptr;
	if (myTimerAttr && myTimerAttr->callback)
	{
		myTimerAttr->callback(myTimerAttr->user_data);
	}
}

myTimer::myTimer()
{

}

myTimer::~myTimer()
{
    

}

// 创建定时器
bool myTimer::create_timer(void (*callback)(void *), void *user_data)
{
	this->is_running = false;

    timerAttr myTimerAttr;

    myTimerAttr.callback = callback;
    myTimerAttr.user_data = user_data;

	// 配置定时器属性
	struct sigevent sev;
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = timer_signal_handler;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = &myTimerAttr;

	// 创建POSIX定时器
	if (timer_create(CLOCK_REALTIME, &sev, &this->timer_id) == -1)
	{
		return false;
	}

	return true;
}

// 启动定时器
int myTimer::timer_start(long interval_sec, long interval_nsec, bool repeat)
{
	// 设置初始触发时间和间隔时间
	this->spec.it_value.tv_sec = interval_sec;
	this->spec.it_value.tv_nsec = interval_nsec;

	if (repeat)
	{
		this->spec.it_interval.tv_sec = interval_sec;
		this->spec.it_interval.tv_nsec = interval_nsec;
	}
	else
	{
		this->spec.it_interval.tv_sec = 0;
		this->spec.it_interval.tv_nsec = 0;
	}

	// 启动定时器
	if (timer_settime(this->timer_id, 0, &this->spec, NULL) == -1)
	{
		return -1;
	}

	this->is_running = true;
	return 0;
}

// 停止定时器
int myTimer::timer_stop()
{
	// 设置定时器时间为0来停止
	struct itimerspec stop_spec = {{0, 0}, {0, 0}};
	if (timer_settime(this->timer_id, 0, &stop_spec, NULL) == -1)
	{
		return -1;
	}

	this->is_running = false;
	return 0;
}

// 重启定时器
int myTimer::timer_restart()
{
	if (timer_settime(this->timer_id, 0, &this->spec, NULL) == -1)
	{
		return -1;
	}

	this->is_running = true;
	return 0;
}

// 检查定时器是否正在运行
bool myTimer::timer_is_running()
{
	return this->is_running;
}

// 销毁定时器
void myTimer::timer_destroy()
{
	if (timer_id > 0)
	{
		timer_stop();
		timer_delete(timer_id);
	}
}
