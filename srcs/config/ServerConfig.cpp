#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
{
	this->_port = 0;
	this->_ports.push_back(this->_port);
	this->_host = "127.0.0.1";
	this->_root = ".";
	this->_clientMaxBodySize = 1048576;
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
		this->_ports = other._ports;
		this->_host = other._host;
		this->_root = other._root;
		this->_serverNames = other._serverNames;
		this->_errorPages = other._errorPages;
		this->_clientMaxBodySize = other._clientMaxBodySize;
		this->_locations = other._locations;
		this->_index = other._index;
	}
	return *this;
}

ServerConfig::~ServerConfig() {}

bool	ServerConfig::setPort(int port)
{
	for (size_t i = 0; i < this->_ports.size(); ++i)
	{
		if (this->_ports[i] == port)
			return false;
	}
	this->_ports.push_back(port);
	this->_port = port;
	return true;
}

void	ServerConfig::setHost(const std::string& host)
{
	this->_host = host;
}

void	ServerConfig::setRoot(const std::string& root)
{
	this->_root = root;
}

void	ServerConfig::addIndex(const std::string& indexPage)
{
	this->_index.push_back(indexPage);
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

int	ServerConfig::getPort() const
{
	return this->_port;
}

const std::string& ServerConfig::getHost() const
{
	return this->_host;
}

const std::string& ServerConfig::getRoot() const
{
	return this->_root;
}

const std::vector<std::string>& ServerConfig::getIndex() const
{
	return this->_index;
}

const std::vector<std::string>& ServerConfig::getServerNames() const
{
	return this->_serverNames;
}

std::string ServerConfig::getErrorPages(int statusCode) const
{
	std::map<int, std::string>::const_iterator it = _errorPages.find(statusCode);
	
	if (it != _errorPages.end())
		return (it->second);
	return ("");
}

size_t ServerConfig::getClientMaxBodySize() const
{
	return this->_clientMaxBodySize;
}

const std::vector<Location>& ServerConfig::getLocations() const
{
	return this->_locations;
}

const Location* ServerConfig::matchLocation(const std::string& uri) const
{
	const Location* matchedLocation = NULL;
	size_t maxMatchLength = 0;

	for (size_t i = 0; i < _locations.size(); ++i)
	{
		const std::string& locPath = _locations[i].getPath();
		if (uri.find(locPath) == 0)
		{
			bool isDirectoryMatch = (uri.length() == locPath.length()) || 
									(!locPath.empty() && locPath[locPath.length() - 1] == '/') ||
									(uri[locPath.length()] == '/');
				
			if (isDirectoryMatch && locPath.length() > maxMatchLength)
			{
				maxMatchLength = locPath.length();
				matchedLocation = &_locations[i];
			}
			else if (locPath.length() > 0 && locPath[locPath.length() - 1] == '/')
			{
				std::string locPathNoSlash = locPath.substr(0, locPath.length() - 1);
				if (uri == locPathNoSlash && locPath.length() > maxMatchLength)
				{
					maxMatchLength = locPath.length();
					matchedLocation = &_locations[i];
				}
			}
		}
	}
	return matchedLocation;
}
