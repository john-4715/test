
#include <arpa/inet.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *hostname = "10.166.64.33";
	const char *username = "root";
	const char *password = "12345678";
	const char *remote_file = "/home/hejr/template.json";
	const char *local_file = "downloaded.json";

	LIBSSH2_SESSION *session;
	LIBSSH2_SFTP *sftp_session;
	LIBSSH2_SFTP_HANDLE *sftp_handle;

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(22);
	sin.sin_addr.s_addr = inet_addr(hostname);

	// 连接服务器
	if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)))
	{
		perror("连接失败");
		return -1;
	}

	// 初始化SSH会话
	libssh2_init(0);
	session = libssh2_session_init();

	if (libssh2_session_handshake(session, sock))
	{
		fprintf(stderr, "SSH握手失败\n");
		return -1;
	}

	// 用户认证
	if (libssh2_userauth_password(session, username, password))
	{
		fprintf(stderr, "认证失败\n");
		return -1;
	}

	// 初始化SFTP会话
	sftp_session = libssh2_sftp_init(session);
	if (!sftp_session)
	{
		fprintf(stderr, "SFTP初始化失败\n");
		return -1;
	}

	// 打开远程文件
	sftp_handle = libssh2_sftp_open(sftp_session, remote_file, LIBSSH2_FXF_READ, 0);
	if (!sftp_handle)
	{
		fprintf(stderr, "无法打开远程文件\n");
		return -1;
	}

	// 创建本地文件
	FILE *local_fp = fopen(local_file, "wb");
	if (!local_fp)
	{
		perror("无法创建本地文件");
		return -1;
	}

	// 读取远程文件并写入本地
	char buffer[1024];
	int rc;
	while ((rc = libssh2_sftp_read(sftp_handle, buffer, sizeof(buffer))) > 0)
	{
		fwrite(buffer, 1, rc, local_fp);
	}

	// 清理资源
	fclose(local_fp);
	libssh2_sftp_close(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
	libssh2_session_disconnect(session, "正常关闭");
	libssh2_session_free(session);
	close(sock);
	libssh2_exit();

	printf("文件下载成功: %s -> %s\n", remote_file, local_file);
	return 0;
}
