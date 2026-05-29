
#ifndef TIMER_H
#define TIMER_H

#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <mutex>

// 定时器结构体
class myTimer
{
public:
	myTimer();
	~myTimer();
	bool create_timer(void (*callback)(void *), void *user_data);
	int timer_start(long interval_sec, long interval_nsec, bool repeat);
	int timer_stop();
	int timer_restart();
	bool timer_is_running();
	void timer_destroy();

private:
	timer_t timer_id;		// 定时器ID
	struct itimerspec spec; // 定时器时间规格
	bool is_running; // 运行状态
    #include <mutex>
};

#endif // TIMER_H
