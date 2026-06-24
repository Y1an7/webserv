#include "ConfigParser.hpp"
#include <cctype>

ConfigParser::ConfigParser()
{
}

ConfigParser::~ConfigParser()
{
}

void ConfigParser::tokenize(const std::string& filename)
{
	std::ifstream	file(filename.c_str());
	std::string		currentToken;
	char			c;

	if (!file.is_open())
	{
		std::cerr << "Error: Cannot open configuration file." << std::endl;
		return ;
	}
	while (file.get(c))
	{
		if (std::isspace(c))
		{
			if (!currentToken.empty())
				this->_tokens.push_back(currentToken);
			currentToken.clear();
		}
		else if (c == '{' || c == '}' || c == ';')
		{
			if (!currentToken.empty())
				this->_tokens.push_back(currentToken);
			currentToken.clear();
			this->_tokens.push_back(std::string(1, c));
		}
		else
			currentToken += c;
	}
	if (!currentToken.empty())
	{
		this->_tokens.push_back(currentToken);
	}
}