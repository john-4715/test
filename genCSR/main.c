#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void handle_errors()
{
	ERR_print_errors_fp(stderr);
	abort();
}

int main()
{
	// 初始化 OpenSSL 库
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	// 生成 RSA 密钥对
	RSA *rsa = RSA_new();
	BIGNUM *e = BN_new();
	BN_set_word(e, RSA_F4);
	if (RSA_generate_key_ex(rsa, 2048, e, NULL) != 1)
	{
		handle_errors();
	}

	// 创建 X509_REQ 对象
	X509_REQ *req = X509_REQ_new();
	if (req == NULL)
	{
		handle_errors();
	}

	// 设置 CSR 的版本
	if (X509_REQ_set_version(req, 1) != 1)
	{
		handle_errors();
	}

	// 创建 X509_NAME 对象并设置主题信息
	X509_NAME *name = X509_REQ_get_subject_name(req);

	char *emailAddress = "John.He@copeland.com"; // email address

	X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"CN", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, (unsigned char *)"Shanxi", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC, (unsigned char *)"Xi'an", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"Copeland", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, (unsigned char *)"IT", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"copeland.local", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "emailAddress", MBSTRING_ASC, (unsigned char *)emailAddress, -1, -1, 0);

	// 设置公钥
	EVP_PKEY *pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pkey, rsa);
	if (X509_REQ_set_pubkey(req, pkey) != 1)
	{
		handle_errors();
	}

	// 使用私钥签名 CSR
	if (X509_REQ_sign(req, pkey, EVP_sha256()) <= 0)
	{
		handle_errors();
	}

	// 将 CSR 保存到内存中
	BIO *bio = BIO_new(BIO_s_mem());
	if (PEM_write_bio_X509_REQ(bio, req) != 1)
	{
		handle_errors();
	}

	// 获取内存中的 CSR 数据
	char *csr_data;
	long csr_len = BIO_get_mem_data(bio, &csr_data);

	// 打印 CSR 数据
	printf("Generated CSR:\n%s", csr_data);

	// 清理资源
	BIO_free(bio);
	X509_REQ_free(req);
	EVP_PKEY_free(pkey);
	// RSA_free(rsa);
	BN_free(e);
	EVP_cleanup();
	ERR_free_strings();

	return 0;
}