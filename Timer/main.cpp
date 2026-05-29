
#include "timer.h"
#include <iostream>
#include <unistd.h>

// 周期性定时器回调函数
void periodic_callback(void *user_data) { std::cout << "周期性定时器触发: " << time(NULL) << std::endl; }

// 单次定时器回调函数
void single_callback(void *user_data) { std::cout << "单次定时器触发: " << time(NULL) << std::endl; }

// 带参数的定时器回调函数
void param_callback(void *user_data)
{
	int *counter = static_cast<int *>(user_data);
	(*counter)++;
	std::cout << "带参数定时器触发，计数: " << *counter << std::endl;
}

int main()
{
	std::cout << "开始测试C++定时器..." << std::endl;

	// 创建周期性定时器(每1秒触发)
	Timer periodic_timer(1000, periodic_callback);

	// 创建单次定时器(3秒后触发)
	Timer single_timer(3000, single_callback, NULL, true);

	// 创建带参数的定时器
	int counter = 0;
	Timer param_timer(500, param_callback, &counter);

	// 启动所有定时器
	std::cout << "启动所有定时器..." << std::endl;
	periodic_timer.start();
	single_timer.start();
	param_timer.start();

	// 检查定时器状态
	std::cout << "周期性定时器状态: " << (periodic_timer.is_active() ? "运行中" : "已停止") << std::endl;
	std::cout << "单次定时器状态: " << (single_timer.is_active() ? "运行中" : "已停止") << std::endl;
	std::cout << "带参数定时器状态: " << (param_timer.is_active() ? "运行中" : "已停止") << std::endl;

	// 运行5秒
	std::cout << "定时器运行5秒..." << std::endl;
	sleep(5);

	// 停止周期性定时器
	std::cout << "停止周期性定时器..." << std::endl;
	periodic_timer.stop();

	// 再运行2秒
	std::cout << "继续运行2秒..." << std::endl;
	sleep(2);

	std::cout << "测试完成" << std::endl;
	return 0;
}
