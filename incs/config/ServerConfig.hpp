#ifndef SERVERCONFIG_H
# define SERVERCONFIG_H

# include <string>
# include <iostream>
# include <vector>
# include <map>
# include "Location.hpp"

class ServerConfig
{
private:
	int							_port; //tell sockets port to listen
	std::string					_host; //tell socket ip address
	std::vector<std::string>	_serverNames; //when two websites share the same ip and port, look at the host header in the user's http request and matches it against serverNames
	std::map<int, std::string>	_errorPages; //server check the map to search 
	size_t						_clientMaxBodySize; //security check to prevent out of memory and crashing
	std::vector<Location>		_locations; // a list of specific routing rules

public:
	ServerConfig();
	ServerConfig(const ServerConfig& other);
	ServerConfig& operator=(const ServerConfig& other);
	~ServerConfig();

	//setters
	void	setPort(int port);
	void	setHost(const std::string& host);
	void	addServerNames(const std::string& serverNames);
	void	setErrorPages(int code, const std::string& error);
	void	setClientMaxBodySize(size_t size);
	void	addLocations(const Location& loc);
};

#endif