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
			if (!currentToken.empty())
			{
				this->_tokens.push_back(currentToken);
				currentToken.clear();
			}
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
		else if (this->_tokens[this->_pos] == "server_name")
			this->parseServerName(newServer);
		else if (this->_tokens[this->_pos] == "client_max_body_size")
			this->parseClientMaxBodySize(newServer);
		else if (this->_tokens[this->_pos] == "location")
		{
			this->_pos++;
			this->parseLocationBlock(newServer);
		}
	
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

	std::string portStr = this->_tokens[this->_pos]; //store the port string
	size_t i = 0;
	while (i < portStr.length())
	{
		if (!std::isdigit(portStr[i]))
			throw ConfigParser::SyntaxException();
		i++;
	}

	int portNumber;
	std::stringstream ss(portStr); //convert a string into an actual integer in C++98
	ss >> portNumber;

	if (portNumber <= 0 || portNumber > 65535) //this number is 2^16 -1
		throw ConfigParser::SyntaxException();
	server.setPort(portNumber);
	this->_pos++;
	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != ";")
		throw ConfigParser::SyntaxException();
	this->_pos++;
}

void	ConfigParser::parseServerName(ServerConfig& server)
{
	this->_pos++;
	while (this->_pos < this->_tokens.size() && this->_tokens[this->_pos] != ";")
	{
		server.addServerNames(this->_tokens[this->_pos]);
		this->_pos++;
	}

	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != ";")
		throw ConfigParser::SyntaxException();
	this->_pos++;
}

void	ConfigParser::parseClientMaxBodySize(ServerConfig& server)
{
	this->_pos++;
	if (this->_pos >= this->_tokens.size())
		throw ConfigParser::SyntaxException();

	std::string token = this->_tokens[this->_pos];
	size_t i = 0;
	size_t multiplier = 1;

	while (i < token.length())
	{
		if (!std::isdigit(token[i]))
		{
			if (i == token.size() - 1)
			{
				if (token[i] == 'm' || token[i] == 'M')
					multiplier = 1024 * 1024;
				else if (token[i] == 'k' || token[i] == 'K')
					multiplier = 1024;
				else
					throw ConfigParser::SyntaxException();
			}
			else
				throw ConfigParser::SyntaxException();
		}
		i++;
	}

	std::string pureNumberStr = token;
	if (multiplier != 1)
		pureNumberStr = token.substr(0, token.length() - 1);
	
	size_t parsedNumber;
	std::stringstream ss(pureNumberStr);
	ss >> parsedNumber;

	size_t maxSize = ~(size_t(0));
	if (multiplier > 0 && parsedNumber > (maxSize / multiplier))
		throw ConfigParser::SyntaxException();

	size_t	finalSize = parsedNumber * multiplier;
	server.setClientMaxBodySize(finalSize);

	this->_pos++;
	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != ";")
		throw	ConfigParser::SyntaxException();
	this->_pos++;
}

void	ConfigParser::parseLocationBlock(ServerConfig& server)
{
	Location	newLocation;
	std::string	directive;

	if (this->_pos >= this->_tokens.size())
		throw ConfigParser::SyntaxException();
	newLocation.setPath(this->_tokens[this->_pos]);
	this->_pos++;

	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != "{")
		throw ConfigParser::SyntaxException();
	this->_pos++;

	while (this->_pos < this->_tokens.size() && this->_tokens[this->_pos] != "}")
	{
		directive = this->_tokens[this->_pos];
		this->_pos++;

		if (directive == "root")
		{
			if (this->_pos >= this->_tokens.size())
				throw ConfigParser::SyntaxException();
			newLocation.setRoot(this->_tokens[this->_pos]);
			this->_pos++;
		}
		else if (directive == "autoindex")
		{
			if (this->_pos >= this->_tokens.size())
				throw ConfigParser::SyntaxException();
			if (this->_tokens[this->_pos] == "on")
				newLocation.setAutoindex(true);
			else if (this->_tokens[this->_pos] == "off")
				newLocation.setAutoindex(false);
			else
				throw ConfigParser::SyntaxException();
			this->_pos++;
		}
		else if (directive == "index")
		{
			while (this->_pos < this->_tokens.size() && this->_tokens[this->_pos] != ";")
			{
				newLocation.addIndex(this->_tokens[this->_pos]);
				this->_pos++;
			}
		}
		else if (directive == "methods")
		{
			while (this->_pos < this->_tokens.size() && this->_tokens[this->_pos] != ";")
			{
				newLocation.addMethod(this->_tokens[this->_pos]);
				this->_pos++;
			}
		}
		else if (directive == "cgi_extension")
		{
			if (this->_pos >= this->_tokens.size())
				throw ConfigParser::SyntaxException();
			newLocation.setCgiExtension(this->_tokens[this->_pos]);
			this->_pos++;
		}
		else if (directive == "cgi_path")
		{
			if (this->_pos >= this->_tokens.size())
				throw ConfigParser::SyntaxException();
			newLocation.setCgiPath(this->_tokens[this->_pos]);
			this->_pos++;
		}
		else
			throw ConfigParser::SyntaxException();
		
		if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != ";")
			throw ConfigParser::SyntaxException();
		this->_pos++;
	}
	if (this->_pos >= this->_tokens.size() || this->_tokens[this->_pos] != "}")
		throw ConfigParser::SyntaxException();
	this->_pos++;

	server.addLocations(newLocation);
}
