

#include "test.h"
#include "json_batchallocator.h"
#include "jsontest.h"
#include <json/autolink.h>
#include <json/config.h>
#include <json/features.h>
#include <json/forwards.h>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>


void test_json()
{
	// 创建一个Json::Value对象，它可以是数组或对象
	Json::Value root;
	Json::Value item;

	// 添加一些数据到对象中
	root["name"] = "John Doe";
	root["age"] = 30;
	root["is_student"] = false;

	// 添加一个数组到对象中
	Json::Value skills;
	skills.append("C++");
	skills.append("Python");
	skills.append("Java");
	root["skills"] = skills;

	std::string val = root["age"].asString();
	// 将对象转换为字符串形式输出
	Json::StyledWriter builder;
	const std::string json_str = builder.write(root);
	std::cout << json_str << std::endl;
}

int json_noexist()
{
	Json::Value root;
	Json::Reader reader;
	std::string errs;

	std::string json_str = R"({"name": "hejr", "age": "30"})";
	bool parsingSuccessful = reader.parse(json_str, root, &errs);
	if (!parsingSuccessful)
	{
		std::cout << "Failed to parse JSON: " << errs << std::endl;
		return -1;
	}
	int age = root["age"].asInt();
	
	std::cout << "age: " << age << std::endl;

    std::string name = root["name"].asString(); // 正确读取字符串
	int id = root["id"].asInt();				// 如果id不存在，这里会抛出异常
	std::cout << "ID: " << id << std::endl;		// 这行不会执行到
	return 0;
}

int func_add(int a, int b)
{
	int *pTemp = NULL;
	*pTemp = 0x02;

	return a + b;
}

void func_a(int a)
{
	printf("%d: A call B\n", a);
	func_b(2);
}

void func_b(int b)
{
	printf("%d: B call C\n", b);
	func_c(3); /* 这个函数调用将导致段错误 */
}

void func_c(int c)
{
	char *p = (char *)c;
	*p = 'A'; /* 如果参数c不是一个可用的地址值，则这条语句导致段错误 */
	printf("%d: function C\n", c);
}