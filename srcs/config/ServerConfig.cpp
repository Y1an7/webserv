#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
{
	this->_port = 0;
	this->_host = "";
	this->_clientMaxBodySize = 0;
}

ServerConfig::ServerConfig(const ServerConfig& other)
{
	*this = other;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other)
{
	if (this != &other)
	{
		this->_port = other._port;
		this->_host = other._host;
		this->_serverNames = other._serverNames;
		this->_errorPages = other._errorPages;
		this->_clientMaxBodySize = other._clientMaxBodySize;
		this->_locations = other._locations;
	}
	return *this;
}

ServerConfig::~ServerConfig() {}

void	ServerConfig::setPort(int port)
{
	this->_port = port;
}

void	ServerConfig::setHost(const std::string& host)
{
	this->_host = host;
}

void	ServerConfig::addServerNames(const std::string& serverNames)
{
	this->_serverNames.push_back(serverNames);
}

void	ServerConfig::setErrorPages(int code, const std::string& error)
{
	this->_errorPages[code] = error;
}

void	ServerConfig::setClientMaxBodySize(size_t size)
{
	this->_clientMaxBodySize = size;
}

void	ServerConfig::addLocations(const Location& loc)
{
	this->_locations.push_back(loc);
}