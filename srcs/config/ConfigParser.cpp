#include "ConfigParser.hpp"
#include <cctype>

ConfigParser::ConfigParser()
{
	this->_pos = 0;
}

ConfigParser::ConfigParser(const ConfigParser& other)
{
	*this = other;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		this->_pos = other._pos;
		this->_tokens = other._tokens;
	}
	return *this;
}

ConfigParser::~ConfigParser() {}

void ConfigParser::tokenize(const std::string& filename)
{
	std::ifstream	file(filename.c_str());
	std::string		currentToken;
	char			c;

	if (!file.is_open())
	{
		throw std::runtime_error("Error: Cannot open configuration file.");
	}
	while (file.get(c))
	{
		if (c == '#')
		{
			while (file.get(c) && c != '\n' )
			{}
			continue ;
		}
		if (std::isspace(c))
		{
			if (!currentToken.empty())
			{
				this->_tokens.push_back(currentToken);
				currentToken.clear();
			}
		}
		else if (c == '{' || c == '}' || c == ';')
		{
			if (!currentToken.empty())
			{
				this->_tokens.push_back(currentToken);
				currentToken.clear();
			}
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

const char* ConfigParser::SyntaxException::what() const throw()
{
	return "Error: Config file syntax error.";
}

void	ConfigParser::parse()
{
	
}