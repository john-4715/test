#include <iostream>
#include <sstream>
#include <string>

std::string addEscapeCharacters(const std::string &input)
{
	std::ostringstream escaped;
	for (char c : input)
	{
		switch (c)
		{
		case '\n':
			escaped << "\\n";
			break;
		case '\t':
			escaped << "\\t";
			break;
		case '\'':
			escaped << "\\'";
			break;
		case '\"':
			escaped << "\\\"";
			break;
		case '\\':
			escaped << "\\\\";
			break;
		default:
			escaped << c;
			break;
		}
	}
	return escaped.str();
}

int main()
{
	std::string original = "Hello\"World!\"";
	std::string escaped = addEscapeCharacters(original);
	std::cout << "Original: " << original << std::endl;
	std::cout << "Escaped: " << escaped << std::endl;
	return 0;
}