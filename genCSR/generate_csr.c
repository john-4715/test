

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <stdlib.h>

void handle_errors()
{
	ERR_print_errors_fp(stderr);
	abort();
}

int main()
{
	// Initializes the OpenSSL library
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	// Generate an RSA key pair
	RSA *rsa = RSA_new();
	BIGNUM *e = BN_new();
	BN_set_word(e, RSA_F4);
	if (RSA_generate_key_ex(rsa, 2048, e, NULL) != 1)
	{
		handle_errors();
	}

	// Create an X509_REQ object
	X509_REQ *req = X509_REQ_new();
	if (!req)
	{
		handle_errors();
	}

	// Set the version of X509_REQ
	if (X509_REQ_set_version(req, 2) != 1)
	{
		handle_errors();
	}

	// Create the X509_NAME object and set the theme information
	X509_NAME *name = X509_NAME_new();
	if (!name)
	{
		handle_errors();
	}

	char *emailAddress = "John.He@copeland.com"; // email address

	X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"CN", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, (unsigned char *)"Shanxi", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC, (unsigned char *)"Xi'an", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"Copeland", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, (unsigned char *)"IT", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"copeland.local", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "emailAddress", MBSTRING_ASC, (unsigned char *)emailAddress, -1, -1, 0);

	// Set the theme for X509_REQ
	if (X509_REQ_set_subject_name(req, name) != 1)
	{
		handle_errors();
	}

	// Set the public key of X509_REQ
	EVP_PKEY *pkey = EVP_PKEY_new();
	if (!pkey)
	{
		handle_errors();
	}
	EVP_PKEY_assign_RSA(pkey, rsa);

	if (X509_REQ_set_pubkey(req, pkey) != 1)
	{
		handle_errors();
	}

	// The CSR is signed using the private key
	if (X509_REQ_sign(req, pkey, EVP_sha256()) <= 0)
	{
		handle_errors();
	}

	// Write the CSR to the PEM file
	FILE *pem_file = fopen("example.csr", "w");
	if (!pem_file)
	{
		perror("Unable to open file for writing");
		exit(EXIT_FAILURE);
	}

	if (PEM_write_X509_REQ(pem_file, req) != 1)
	{
		handle_errors();
	}

	fclose(pem_file);

	// Release resources
	X509_NAME_free(name);
	X509_REQ_free(req);
	EVP_PKEY_free(pkey);
	// RSA_free(rsa);

	BN_free(e);

	// Clear the OpenSSL library
	EVP_cleanup();
	ERR_free_strings();
	CRYPTO_cleanup_all_ex_data();

	printf("CSR generated successfully: example.csr\n");

	return 0;
}