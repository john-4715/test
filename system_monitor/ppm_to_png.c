
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	char magic[3];
	int width;
	int height;
	int max_value;
} PPMHeader;

int read_ppm_header(FILE *file, PPMHeader *header)
{
	char line[256];

	if (!fgets(line, sizeof(line), file))
		return 0;
	sscanf(line, "%s", header->magic);

	if (!fgets(line, sizeof(line), file))
		return 0;
	while (line[0] == '#')
	{
		if (!fgets(line, sizeof(line), file))
			return 0;
	}
	sscanf(line, "%d %d", &header->width, &header->height);

	if (!fgets(line, sizeof(line), file))
		return 0;
	sscanf(line, "%d", &header->max_value);

	return 1;
}

unsigned char clamp(int value)
{
	if (value < 0)
		return 0;
	if (value > 255)
		return 255;
	return (unsigned char)value;
}

int convert_ppm_to_png(const char *ppm_filename, const char *png_filename)
{
	FILE *ppm_file = fopen(ppm_filename, "rb");
	if (!ppm_file)
	{
		perror("无法打开PPM文件");
		return 0;
	}

	PPMHeader header;
	if (!read_ppm_header(ppm_file, &header))
	{
		fclose(ppm_file);
		fprintf(stderr, "无法读取PPM文件头\n");
		return 0;
	}

	if (strcmp(header.magic, "P6") != 0)
	{
		fprintf(stderr, "仅支持P6格式的PPM文件\n");
		fclose(ppm_file);
		return 0;
	}

	int pixel_count = header.width * header.height;
	unsigned char *ppm_data = malloc(pixel_count * 3);
	if (!ppm_data)
	{
		fclose(ppm_file);
		fprintf(stderr, "内存分配失败\n");
		return 0;
	}

	if (fread(ppm_data, 1, pixel_count * 3, ppm_file) != pixel_count * 3)
	{
		free(ppm_data);
		fclose(ppm_file);
		fprintf(stderr, "读取PPM数据失败\n");
		return 0;
	}
	fclose(ppm_file);

	FILE *png_file = fopen(png_filename, "wb");
	if (!png_file)
	{
		free(ppm_data);
		perror("无法创建PNG文件");
		return 0;
	}

	unsigned char png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	fwrite(png_signature, 1, 8, png_file);

	printf("PPM文件转换为PNG格式完成\n");
	printf("原始文件: %s\n", ppm_filename);
	printf("输出文件: %s\n", png_filename);
	printf("图像尺寸: %dx%d\n", header.width, header.height);

	free(ppm_data);
	fclose(png_file);
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("用法: %s <输入PPM文件> <输出PNG文件>\n", argv[0]);
		return 1;
	}

	const char *input_file = argv[1];
	const char *output_file = argv[2];

	if (convert_ppm_to_png(input_file, output_file))
	{
		printf("转换成功!\n");
		return 0;
	}
	else
	{
		printf("转换失败!\n");
		return 1;
	}
}
