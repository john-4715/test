#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOST "localhost"
#define PORT 1883
#define KEEP_ALIVE 60
#define MSG_MAX_SIZE 512
#define TOPIC_NUM 3

bool session = true;

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{

	if (message->payloadlen)
	{
		printf("%s %s", message->topic, (char *)message->payload);
	}
	else
	{
		printf("%s (null)\n", message->topic);
	}
	fflush(stdout);
}

void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	if (!result)
	{
		/* Subscribe to broker information topics on successful connect. */
		mosquitto_subscribe(mosq, NULL, "topic1 ", 2);
	}
	else
	{
		fprintf(stderr, "Connect failed\n");
	}
}

void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;
	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for (i = 1; i < qos_count; i++)
	{
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	printf("%s\n", str);
}

int main()
{
	struct mosquitto *mosq = NULL;
	char buff[MSG_MAX_SIZE];

	// libmosquitto 库初始化
	mosquitto_lib_init();
	// 创建mosquitto客户端
	mosq = mosquitto_new(NULL, session, NULL);
	if (!mosq)
	{
		printf("create client failed..\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	// 设置回调函数，需要时可使用
	mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

	// 连接服务器
	if (mosquitto_connect(mosq, HOST, PORT, KEEP_ALIVE))
	{
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}
	// 开启一个线程，在线程里不停的调用 mosquitto_loop() 来处理网络信息
	int loop = mosquitto_loop_start(mosq);
	if (loop != MOSQ_ERR_SUCCESS)
	{
		printf("mosquitto loop error\n");
		return 1;
	}

	while (fgets(buff, MSG_MAX_SIZE, stdin) != NULL)
	{
		/*发布消息*/
		mosquitto_publish(mosq, NULL, "topic2 ", strlen(buff) + 1, buff, 0, 0);
		memset(buff, 0, sizeof(buff));
	}

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return 0;
}