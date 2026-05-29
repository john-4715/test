
#include <iostream>
#include <regex>
#include <string>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

class EmailValidator
{
private:
	std::regex emailPattern;

public:
	EmailValidator() : emailPattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$") {}

	bool isValidChar(char c)
	{
		return std::isalnum(c) || c == '@' || c == '.' || c == '_' || c == '%' || c == '+' || c == '-' || c == '!' || c == '#' || c == '$' ||
			   c == '&' || c == '*' || c == '/' || c == '=' || c == '?' || c == '^' || c == '`' || c == '{' || c == '|' || c == '}' || c == '~';
	}

	bool isValidEmail(const std::string &email)
	{
		if (email.empty())
			return false;
		return std::regex_match(email, emailPattern);
	}

	bool isPotentiallyValid(const std::string &partialEmail)
	{
		if (partialEmail.empty())
			return true;

		// 检查是否可能构成有效邮箱地址
		// 允许的字符组合
		for (char c : partialEmail)
		{
			if (!isValidChar(c))
			{
				return false;
			}
		}

		// 检查@符号的数量
		int atCount = 0;
		for (char c : partialEmail)
		{
			if (c == '@')
				atCount++;
		}
		if (atCount > 1)
			return false;

		// 检查域名部分是否可能有效
		size_t atPos = partialEmail.find('@');
		if (atPos != std::string::npos)
		{
			// @符号后面的部分
			if (atPos + 1 < partialEmail.length())
			{
				std::string domainPart = partialEmail.substr(atPos + 1);
				
				// 检查域名部分是否包含至少一个点号
				if (domainPart.find('.') == std::string::npos && !domainPart.empty())
				{
					return true; // 可能是域名的开始
				}
			}
		}

		return true;
	}
};

class InputHandler
{
private:
	struct termios oldTermios;

public:
	InputHandler()
	{
		struct termios newTermios;
		tcgetattr(STDIN_FILENO, &oldTermios);
		newTermios = oldTermios;
		newTermios.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
	}

	~InputHandler() { tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios); }

	char getch()
	{
		char ch;
		if (read(STDIN_FILENO, &ch, 1) < 0)
		{
			return 0;
		}
		return ch;
	}

	bool kbhit()
	{
		fd_set fds;
		struct timeval tv = {0, 0};
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
	}
};

void clearLine() { std::cout << "\r\033[K" << std::flush; }

void displayResult(const std::string &email, EmailValidator &validator)
{
	clearLine();
	std::cout << "输入: " << email;

	if (email.empty())
	{
		std::cout << " (请输入邮箱地址)" << std::flush;
		return;
	}

	if (validator.isValidEmail(email))
	{
		std::cout << " \033[32m✓ 有效邮箱\033[0m" << std::flush;
	}
	else if (validator.isPotentiallyValid(email))
	{
		std::cout << " \033[33m? 输入中...\033[0m" << std::flush;
	}
	else
	{
		std::cout << " \033[31m✗ 格式错误\033[0m" << std::flush;
	}
}

int main()
{
	EmailValidator validator;
	InputHandler inputHandler;
	std::string email;

	std::cout << "邮箱地址实时校验程序 (按ESC退出)\n";
	std::cout << "====================================\n";
	displayResult(email, validator);

	while (true)
	{
		if (inputHandler.kbhit())
		{
			char ch = inputHandler.getch();
			std::cout << "ch = " << static_cast<int>(ch);
			if (ch == 27)
			{ // ESC键
				clearLine();
				std::cout << "程序退出\n" << std::flush;
				break;
			}
			else if (ch == 127 || ch == 8)
			{ // 退格键
				if (!email.empty())
				{
					email.pop_back();
					displayResult(email, validator);
				}
			}
			else if (ch == 10 || ch == 13)
			{ // 回车键
				clearLine();
				if (validator.isValidEmail(email))
				{
					std::cout << "最终结果: " << email << " \033[32m✓ 有效邮箱\033[0m\n" << std::flush;
				}
				else
				{
					std::cout << "最终结果: " << email << " \033[31m✗ 无效邮箱\033[0m\n" << std::flush;
				}
				email.clear();
				std::cout << "\n继续输入 (按ESC退出): ";
				displayResult(email, validator);
			}
			else if (validator.isValidChar(ch) && email.length() < 100)
			{
				email += ch;
				displayResult(email, validator);
			}
		}
	}

	return 0;
}
