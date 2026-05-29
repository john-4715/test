#ifndef __HTTP_CLIENT_HPP__
#define __HTTP_CLIENT_HPP__

#include <iostream>
#include <sstream>

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include "event.h"
#include "evhttp.h"

using namespace std;

class evhttpClient
{
private:
	string ip;
	int port;
	string path = "";
	string body = "";
	int timeout = 10;

public:
	/*
		@param path 资源路径可以包含GET请求参数部分
			如"/getparam?param1=123456&param2=456789"
	*/
	evhttpClient(string ip, int port, string path);
	// 直接传入URL进行初始化
	evhttpClient(string url);
	// 添加GET请求参数
	void addGetParam(string name, string value);
	// 设置包体
	void setBody(string body);
	// 返回请求的url
	string getUrl();
	// 设置请求超时时间
	void setTimeout(int timeout);
	/*
		发送GET/POST请求
		@param type http请求类型
		EVHTTP_REQ_GET 1
		EVHTTP_REQ_POST 2
	*/
	bool sendHttpRequest(evhttp_cmd_type type);
	~evhttpClient();

private:
	// 请求回调函数,可以自行在类外实现
	static void requestCallback(struct evhttp_request *request, void *arg);
};

#endif