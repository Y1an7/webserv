#include "ConfigParser.hpp"
#include <cctype>
#include <sstream>
#include <stdexcept>

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
	this->_pos = 0;
	while (this->_pos < _tokens.size())
	{
		if (this->_tokens[_pos] == "server")
		{
			this->_pos++;
			this->parseServerBlock();
		}
		else
		{
			throw ConfigParser::SyntaxException();
		}
	}
}

void	ConfigParser::parseServerBlock()
{
	ServerConfig newServer;

	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != "{")
		throw ConfigParser::SyntaxException();
	this->_pos++;

	while (this->_pos < this->_tokens.size() && this->_tokens[this->_pos] != "}")
	{
		if (this->_tokens[this->_pos] == "listen")
			this->parseListen(newServer);
		else
			throw ConfigParser::SyntaxException();
	}

	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != "}")
		throw ConfigParser::SyntaxException();
	this->_pos++;
	this->_servers.push_back(newServer);
}

void	ConfigParser::parseListen(ServerConfig& server)
{
	this->_pos++;
	if (this->_pos >= this->_tokens.size())
		throw ConfigParser::SyntaxException();

	std::string portStr = this->_tokens[this->_pos];
	size_t i = 0;
	while (i < portStr.length())
	{
		if (!std::isdigit(portStr[i]))
			throw ConfigParser::SyntaxException();
		i++;
	}

	int portNumber;
	std::stringstream ss(portStr);
	ss >> portNumber;

	if (portNumber <= 0 || portNumber > 65535)
		throw ConfigParser::SyntaxException();
	server.setPort(portNumber);
	this->_pos++;
	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != ";")
		throw ConfigParser::SyntaxException();
	this->_pos++;
}
