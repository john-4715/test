#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define MAX_CERTS 4

void print_error(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	ERR_print_errors_fp(stderr);
}

void print_time(ASN1_TIME *time, const char *prefix)
{
	BIO *bio = BIO_new(BIO_s_mem());
	if (!bio)
	{
		print_error("Failed to create BIO");
		return;
	}

	if (ASN1_TIME_print(bio, time))
	{
		char *str;
		long len = BIO_get_mem_data(bio, &str);
		printf("%s: %.*s\n", prefix, (int)len, str);
	}
	else
	{
		print_error("Failed to print time");
	}

	BIO_free(bio);
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <certificate_file> <maxCertNum>\n", argv[0]);
		return 1;
	}

	FILE *file = fopen(argv[1], "r");
	if (!file)
	{
		perror("Failed to open file");
		return 1;
	}

    int maxCertNum = atoi(argv[2]);

	X509 *certs[MAX_CERTS] = {NULL};
	int cert_count = 0;

	// 初始化OpenSSL
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	// 读取证书
	while (cert_count < maxCertNum && cert_count < MAX_CERTS)
	{
		X509 *cert = PEM_read_X509(file, NULL, NULL, NULL);
		if (!cert)
		{
			// 可能到达文件末尾或读取错误
			if (!ERR_peek_error())
			{
				break; // 正常到达文件末尾
			}
			print_error("Error reading certificate");
			break;
		}

		certs[cert_count++] = cert;
	}

	fclose(file);

	if (cert_count == 0)
	{
		fprintf(stderr, "No certificates found in file\n");
		return 1;
	}

	printf("Successfully read %d certificates\n", cert_count);

	// 处理每个证书
	for (int i = 0; i < cert_count; i++)
	{
		if (certs[i])
		{
			printf("\nCertificate %d:\n", i + 1);

			// 获取证书有效期
			ASN1_TIME *not_before = X509_get_notBefore(certs[i]);
			ASN1_TIME *not_after = X509_get_notAfter(certs[i]);

			if (!not_before || !not_after)
			{
				print_error("Failed to get certificate validity period");
				continue;
			}

			print_time(not_before, "Valid from");
			print_time(not_after, "Valid until");
		}

		// 可以在这里添加其他证书信息的提取
	}

	// 清理资源
	for (int i = 0; i < cert_count; i++)
	{
		if (certs[i])
		{
			X509_free(certs[i]);
		}
	}

	EVP_cleanup();
	ERR_free_strings();

	return 0;
}