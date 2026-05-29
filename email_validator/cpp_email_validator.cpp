
#include <iostream>
#include <regex>
#include <string>

class EmailValidator
{
private:
	std::regex emailPattern;

public:
	EmailValidator() : emailPattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$") {}

	bool isValidEmail(const std::string &email)
	{
		if (email.empty())
			return false;
		return std::regex_match(email, emailPattern);
	}
};

int main()
{
	EmailValidator validator;
	std::string testEmail = "john.he@copeland.com";

	if (validator.isValidEmail(testEmail))
	{
		std::cout << "邮箱地址 " << testEmail << " 是合法的" << std::endl;
	}
	else
	{
		std::cout << "邮箱地址 " << testEmail << " 是不合法的" << std::endl;
	}

	return 0;
}
