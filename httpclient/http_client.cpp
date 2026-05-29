#include "http_client.hpp"

evhttpClient::evhttpClient(string ip, int port, string path) : ip(ip), port(port), path(path) {};

evhttpClient::evhttpClient(string url)
{
	struct evhttp_uri *ev_uri = evhttp_uri_parse(url.c_str());
	if (!ev_uri)
	{
		fprintf(stderr, "parse url failed!\n");
		return;
	}
	const char *host = evhttp_uri_get_host(ev_uri);	  // IP
	int p = evhttp_uri_get_port(ev_uri);			  // port
	const char *spath = evhttp_uri_get_path(ev_uri);  // 目录 /tasks
	const char *param = evhttp_uri_get_query(ev_uri); // 获取参数 param1=hello1&param2=hello2

	ip = host;
	port = p;

	if (spath == NULL)
	{
		path = "/";
	}
	else
	{
		if (param == NULL)
		{
			path = spath;
		}
		else
		{
			string sparam = param;
			if (sparam == "") // 参数为空, 不是null
			{
				path = spath;
			}
			else
			{
				stringstream ss;
				ss << spath << "?" << param;
				path = ss.str();
			}
		}
	}
	evhttp_uri_free(ev_uri);
}

void evhttpClient::addGetParam(string name, string value)
{
	if (path.find("?") != string::npos) // 要添加&
	{
		path = path + "&" + name + "=" + value;
	}
	else // 第一个添加 需要?
	{
		path = path + "?" + name + "=" + value;
	}
}

void evhttpClient::setBody(string body) { this->body = body; }

string evhttpClient::getUrl() { return "http://" + ip + ":" + to_string(port) + path; }

void evhttpClient::setTimeout(int timeout) { this->timeout = timeout; }

bool evhttpClient::sendHttpRequest(evhttp_cmd_type type)
{
	// 创建libevent上下文
	struct event_base *base = event_base_new();
	if (!base)
	{
		fprintf(stderr, "Error creating event base\n");
		return false;
	}
	// 创建HTTP客户端
	struct evhttp_connection *conn = evhttp_connection_base_new(base, NULL, ip.c_str(), port);
	if (!conn)
	{
		fprintf(stderr, "Error creating HTTP connection\n");
		event_base_free(base);
		return false;
	}
	// 创建HTTP请求
	struct evhttp_request *req = evhttp_request_new(requestCallback, base);
	if (!req)
	{
		fprintf(stderr, "Error creating HTTP request\n");
		evhttp_connection_free(conn);
		event_base_free(base);
		return false;
	}

	if (type == EVHTTP_REQ_POST)
	{
		/* add body */
		evbuffer_add(req->output_buffer, body.c_str(), body.length());
	}

	// 发送请求
	evhttp_make_request(conn, req, type, path.c_str());
	evhttp_connection_set_timeout(req->evcon, timeout);

	// 运行事件循环
	event_base_dispatch(base);

	// 释放资源
	evhttp_connection_free(conn);
	event_base_free(base);

	return true;
}

evhttpClient::~evhttpClient() {}

void evhttpClient::requestCallback(evhttp_request *request, void *arg)
{
	if (request == NULL)
	{
		fprintf(stderr, "Get Request failed, is NULL");
		return;
	}
	// 获取响应状态码 和 reason
	int response_code = evhttp_request_get_response_code(request);
	std::cout << "Response Code: " << response_code << " " << evhttp_request_get_response_code_line(request)
			  << std::endl;

	// 获取数据长度
	size_t recv_size = evbuffer_get_length(request->input_buffer);
	fprintf(stdout, "Recv Body len(%ld)\n", recv_size);
	if (recv_size > 0)
	{
		// evbuffer_pullup函数是移动指定字节数的数据到huan缓冲区的起始位置
		//  所以取body的时候需要取指定的大小
		string buff = (char *)evbuffer_pullup(request->input_buffer, -1);
		string body = buff.substr(0, recv_size);
		fprintf(stdout, "Recv Body:[%s]\n", body.c_str());
	}
	/*
		event_base_loopexit 函数用于设置一个计数器，告诉事件循环在所有计数器减为0时退出事件循环。
		需要调用 event_base_loopexit 设置计数器的值，并且在每个事件处理完毕时调用 event_base_loopexit 减少计数器的值。
		当计数器减为0时，事件循环会退出。
	*/
	event_base_loopexit((struct event_base *)arg, NULL);
	/*
		event_base_loopbreak
	   函数用于在事件循环中某一次迭代结束后，即使有激活的事件存在，也强制退出事件循环，使事件循环提前终止。
		这个函数会在当前迭代结束后尽快退出事件循环，而不是等待所有的事件都被处理完毕。
		使用 event_base_loopbreak 可以在需要时强制终止事件循环，即使有未处理完的事件存在。
	*/
	// event_base_loopbreak((struct event_base*)arg);
}
