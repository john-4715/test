#include "nand_flash.h"
#include "utils.h"

#define READ_NANDFLASH_FILEPATH "test.img"

int get_device_path(char *filetype, char *device_path)
{
	char *pmtdname = NULL;
	char cmd[256];
	sprintf(cmd, "grep %s /proc/mtd |awk -F ':' '{print $1}'", filetype);
	pmtdname = sfget_shell(cmd);
	if (NULL == pmtdname)
	{

		return 0;
	}
	sprintf(device_path, "/dev/%s", pmtdname);
	sf_free(pmtdname);

	return 1;
}

int write_check_nandflash(char *filetype, char *decry_path)
{
	char device_path[256] = {0};
	char buffer[128] = {0};
	if (get_device_path(filetype, device_path))
	{
		NAND_FLASH_INSTAN *pNand = creare_nandflash_instance();
		if (pNand)
		{
			int nret = pNand->NandErase(device_path, 0);
			nret = pNand->NandWrite(device_path, decry_path, 0);
			unsigned long decry_filelen = get_file_size(decry_path);
			unsigned long fileLen = pNand->NandRead(device_path, READ_NANDFLASH_FILEPATH, 0, decry_filelen);
			release_nandflash_instance();
			if (fileLen < 0)
			{
				printf("read nand flash files error!\n");
				delete_file(READ_NANDFLASH_FILEPATH);
				return 0;
			}

			if (fileLen != decry_filelen)
			{
				printf("read nand flash files length not match!\n");
				delete_file(READ_NANDFLASH_FILEPATH);
				return 0;
			}

			delete_file(READ_NANDFLASH_FILEPATH);
			printf("burn %s to %s success!\n", decry_path, device_path);
		}
	}
}

int main(int argc, char *argv[])
{
	char file_type[32] = "mac";
	char md5String[128] = {0};
	char path[256] = {0};
	char decry_path[256] = {0};
	if (strcmp(file_type, "zimage") == 0)
	{
		write_check_nandflash("mac", decry_path);
	}
	else if (strcmp(file_type, "uboot") == 0)
	{
		write_check_nandflash("mac", decry_path);
	}
	else if (strcmp(file_type, "rbf") == 0)
	{
		write_check_nandflash("mac", decry_path);
	}
	else if (strcmp(file_type, "rootfs") == 0)
	{
		write_check_nandflash("mac", decry_path);
	}
	else if (strcmp(file_type, "mac") == 0)
	{
		write_check_nandflash("mac", decry_path);
	}
	else if (strcmp(file_type, "user") == 0)
	{
		write_check_nandflash("mac", decry_path);
	}
	else if (strcmp(file_type, "applic") == 0)
	{
		write_check_nandflash("mac", decry_path);
	}
	// delete_file(path);
	delete_file(decry_path);
}