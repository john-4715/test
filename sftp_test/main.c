#include <libssh2.h>
#include <libssh2_sftp.h>
 
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
 
int main(int argc, char *argv[]) {
    // 初始化libssh2库
    libssh2_init(0);
 
    // 创建和远程主机的连接
    // 这里需要替换为你的主机地址、用户名和密码
    LIBSSH2_SESSION *session = libssh2_session_init();
    LIBSSH2_SESSION *sftp_session = NULL;
    LIBSSH2_CHANNEL *channel = NULL;
    LIBSSH2_SFTP *sftp_session = NULL;
 
    // 连接到远程服务器
    channel = libssh2_channel_open_session(session);
    if (!channel) {
        // 错误处理
    }
 
    // 激活SFTP会话
    sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        // 错误处理
    }
 
    // 打开远程文件
    LIBSSH2_SFTP_HANDLE *remote_file = libssh2_sftp_open(sftp_session, "remote_file.txt", LIBSSH2_FXF_READ, 0);
    if (!remote_file) {
        // 错误处理
    }
 
    // 创建本地文件
    int fd = open("local_file.txt", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        // 错误处理
    }
 
    // 读取远程文件并写入本地文件
    char buffer[1024];
    unsigned long bytes_read;
    while ((bytes_read = libssh2_sftp_read(remote_file, buffer, sizeof(buffer))) > 0) {
        write(fd, buffer, bytes_read);
    }
 
    // 关闭文件和会话
    libssh2_sftp_close(remote_file);
    libssh2_sftp_shutdown(sftp_session);
    close(fd);
 
    // 清理libssh2资源
    libssh2_session_disconnect(session, "Cleaning up");
    libssh2_session_free(session);
 
    return 0;
}
