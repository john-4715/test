
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* button_test
 */
int main(int argc, char **argv)
{
	int fd;
	unsigned char key_val;

	fd = open("/dev/pl2psIrq0", O_RDWR);
	if (fd < 0)
	{
		printf("can't open!\n");
	}

	while (1)
	{
		printf("ready to read.\n");
		read(fd, &key_val, 1);
		printf("key_val = 0x%x\n", key_val);
	}

	return 0;
}
