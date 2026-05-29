#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <string>
#include <sys/file.h>
#include <unistd.h>

#include <json/autolink.h>
#include <json/config.h>
#include <json/features.h>
#include <json/forwards.h>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

std::string BuildRenewReqBody(std::string deviceGuid, std::string csr)
{
	Json::Value root;

	// 添加字段到root对象
	root["deviceGuid"] = deviceGuid;
	root["csr"] = csr;
	std::string strOut = root.toString();
	return strOut;
}

bool readFileContent(std::string &content, const std::string &filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		printf("Failed to open file:%s\n", filename.c_str());
		return false;
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::string csrContent;
	csrContent.resize(size);
	file.read(&csrContent[0], size);
	content = csrContent;
	file.close();
	return true;
}
// curl --cert client-cert.pem --key client-private.key -X POST
// https://prod.alsenseplatform.com/api/provisioning/v1/provisioning/devices/certificate-renew -H "Content-Type:
// application/json" -d ' {"deviceGuid": "4d2fa0b1-7e71-4507-8175-9798c6....","csr":"-----BEGIN CERTIFICATE
// REQUEST-----\nMIICYDCCAUgCAQAwGzEZMBcGA1UEAwwQbXRscy10ZXN0LWNsaWVudDCCASIwDQYJ\nKoZIhvcN.....\n-----END CERTIFICATE
// REQUEST-----"}'
int main()
{
	std::string csr;
#if 1
	std::string deviceGuid = "bf0654a1-859b-4ad7-b71d-526b3e4293bf";
	std::string filePath = "/data/CertSets/f692caaf54d1aa44db9937292530c978/df4b225d-c249-43c4-a37b-b22ec396ad6f.csr";
	std::string strCmd = "curl -v --tlsv1.2 --tls-max 1.2 --http1.1 --cacert /tmp/PALSandbox/root~3426530195.pem "
						 "--cert /tmp/PALSandbox/cert~3426530195.crt --key /tmp/PALSandbox/private~3426530195.key "
						 "https://gdm-copeland.alsenseplatform.com/v1/provisioning/devices/certificate-renew  -H  "
						 "\"Content-Type: application/json\" -d ";
#else
	std::string deviceGuid = "b7e77db8-e63b-4131-9368-69b5f32eb22b";
	std::string filePath = "/data/CertSets/3fd36d273dc4b7dd736dbecff50c2791/b7e77db8-e63b-4131-9368-69b5f32eb22b.csr";
	std::string strCmd = "curl --cacert /etc/ssl/certs/ca-certificates.crt  --cert /tmp/PALSandbox/cert~311694543.crt "
						 "--key /tmp/PALSandbox/private~311694543.key -X POST "
						 "https://prod.alsenseplatform.com/api/provisioning/v1/provisioning/devices/certificate-renew "
						 "-H \"Content-Type: application/json\" -d ";
#endif
	if (readFileContent(csr, filePath))
	{
		// printf("csr:%s\n", csr.c_str());
	
		std::string body = BuildRenewReqBody(deviceGuid, csr);
		char attach[40960];
		sprintf(attach, "' %s '", body.c_str());
		strCmd += attach;
		std::cout << "strCmd:" << strCmd << std::endl;
		int ret = system(strCmd.c_str());

		if (ret == 0)
		{
			printf("\nCommand executed successfully.\n");
		}
		else
		{
			printf("\nCommand failed to execute. Exit code.\n", ret);
		}
	}
	else
	{
		printf("read csr failed.\n");
	}
	return 0;
}