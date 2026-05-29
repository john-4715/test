#include <iostream>
#include <sstream>

#include "http_client.hpp"


int main()
{
    // evhttpClient client("http://127.0.0.1:7890/getparam?param1=hello1");
    // client.addGetParam("param2", "hello2");
    // cout << client.getUrl() << endl;
    // client.setTimeout(3);
    // cout << "send GET request" << endl;
    // client.sendHttpRequest(EVHTTP_REQ_GET);
 
    evhttpClient client("https://127.0.0.1:8443/setparam");
    string body = "{\"id\":1001,\"name\":\"张三\"}";
    client.setBody(body);
    cout << "send POST request" << endl;
    client.setTimeout(3);
    client.sendHttpRequest(EVHTTP_REQ_POST);
}
