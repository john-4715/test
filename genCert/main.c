#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <stdio.h>
#include <stdlib.h>

void handle_errors()
{
	ERR_print_errors_fp(stderr);
	abort();
}

int main()
{
	// 初始化OpenSSL库
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	// 读取私钥
	FILE *priv_key_file = fopen("private.key", "r");
	if (!priv_key_file)
	{
		perror("Unable to open private key file");
		return 1;
	}
	EVP_PKEY *priv_key = PEM_read_PrivateKey(priv_key_file, NULL, NULL, NULL);
	fclose(priv_key_file);
	if (!priv_key)
	{
		handle_errors();
	}

	// 读取CSR
	FILE *csr_file = fopen("request.csr", "r");
	if (!csr_file)
	{
		perror("Unable to open CSR file");
		return 1;
	}
	X509_REQ *csr = PEM_read_X509_REQ(csr_file, NULL, NULL, NULL);
	fclose(csr_file);
	if (!csr)
	{
		handle_errors();
	}

	// 创建X509证书
	X509 *x509 = X509_new();
	if (!x509)
	{
		handle_errors();
	}

	// 设置证书版本
	X509_set_version(x509, 2);

	// 设置证书序列号
	ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

	// 设置证书有效期
	X509_gmtime_adj(X509_get_notBefore(x509), 0);
	X509_gmtime_adj(X509_get_notAfter(x509), 31536000L); // 1年有效期

	// 设置证书的公钥
	X509_set_pubkey(x509, X509_REQ_get_pubkey(csr));

	// 设置证书的颁发者和主题
	X509_set_issuer_name(x509, X509_REQ_get_subject_name(csr));
	X509_set_subject_name(x509, X509_REQ_get_subject_name(csr));

	// 使用私钥签名证书
	if (!X509_sign(x509, priv_key, EVP_sha256()))
	{
		handle_errors();
	}

	// 将证书写入文件
	FILE *cert_file = fopen("certificate.crt", "w");
	if (!cert_file)
	{
		perror("Unable to open certificate file");
		return 1;
	}
	PEM_write_X509(cert_file, x509);
	fclose(cert_file);

	// 释放资源
	X509_free(x509);
	X509_REQ_free(csr);
	EVP_PKEY_free(priv_key);

	// 清理OpenSSL库
	EVP_cleanup();
	ERR_free_strings();

	printf("Certificate generated successfully.\n");

	return 0;
}