
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// 提取证书的指纹（Subject + Public Key Info的简化哈希或唯一标识）
// 这里为了简化，我们使用证书内容的完整字符串作为唯一标识。
// 在生产环境中，建议使用 OpenSSL 库解析证书并计算 SHA256 指纹。
std::string getCertFingerprint(const std::string &certContent) { return certContent; }

// 从文件中读取所有证书块
// 假设证书以 "-----BEGIN CERTIFICATE-----" 开始，以 "-----END CERTIFICATE-----" 结束
std::vector<std::string> readCertificates(const std::string &filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file " << filename << std::endl;
		return {};
	}

	std::vector<std::string> certs;
	std::string line;
	std::string currentCert;
	bool inCert = false;

	while (std::getline(file, line))
	{
		if (line.find("-----BEGIN CERTIFICATE-----") != std::string::npos)
		{
			inCert = true;
			currentCert = line + "\n";
		}
		else if (line.find("-----END CERTIFICATE-----") != std::string::npos)
		{
			currentCert += line + "\n";
			certs.push_back(currentCert);
			inCert = false;
			currentCert.clear();
		}
		else if (inCert)
		{
			currentCert += line + "\n";
		}
	}
	file.close();
	return certs;
}

// 将证书写入文件
void writeCertificates(const std::string &filename, const std::vector<std::string> &certs)
{
	std::ofstream file(filename, std::ios::trunc); // 清空并写入
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file for writing " << filename << std::endl;
		return;
	}

	for (const auto &cert : certs)
	{
		file << cert;
	}
	file.close();
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv << " <base_ca_file> <new_ca_file>" << std::endl;
		return 1;
	}

	std::string baseFile = argv[1];
	std::string newFile = argv[2];
	std::cout << "baseFile:" << baseFile << std::endl;
	std::cout << "newFile:" << newFile << std::endl;

	// 1. 读取基础证书文件中的所有证书
	std::vector<std::string> baseCerts = readCertificates(baseFile);
	if (baseCerts.empty())
	{
		std::cerr << "Warning: Base file is empty or contains no valid certificates." << std::endl;
	}

	// 2. 建立基础证书的指纹集合，用于快速查找
	std::set<std::string> baseFingerprints;
	for (const auto &cert : baseCerts)
	{
		baseFingerprints.insert(getCertFingerprint(cert));
	}

	// 3. 读取新证书文件中的所有证书
	std::vector<std::string> newCerts = readCertificates(newFile);
	if (newCerts.empty())
	{
		std::cout << "No new certificates found in " << newFile << std::endl;
		return 0;
	}

	// 4. 找出新文件中不在基础文件中的证书
	std::vector<std::string> uniqueNewCerts;
	int addedCount = 0;
	for (const auto &cert : newCerts)
	{
		std::string fp = getCertFingerprint(cert);
		if (baseFingerprints.find(fp) == baseFingerprints.end())
		{
			uniqueNewCerts.push_back(cert);
			baseFingerprints.insert(fp); // 更新集合，避免重复添加（如果新文件自身有重复）
			addedCount++;
		}
	}

	if (addedCount == 0)
	{
		std::cout << "No new unique certificates to add." << std::endl;
		return 0;
	}

	// 5. 合并证书：基础证书 + 新增的唯一证书
	std::vector<std::string> mergedCerts = baseCerts;
	mergedCerts.insert(mergedCerts.end(), uniqueNewCerts.begin(), uniqueNewCerts.end());

	// 6. 写回基础文件
	writeCertificates(baseFile, mergedCerts);

	std::cout << "Successfully added " << addedCount << " new certificates to " << baseFile << std::endl;

	return 0;
}
