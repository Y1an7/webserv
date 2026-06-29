#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <iostream>
# include <string>
# include <vector>
# include <fstream>
# include <exception>
# include "ServerConfig.hpp"


class ConfigParser
{
private:
	std::vector<std::string>	_tokens;
	size_t						_pos;
	std::vector<ServerConfig>	_servers; //for store parsing result

	void						parserServerBlock();
	void						parserListen(ServerConfig& server);

public:
	ConfigParser();
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);
	~ConfigParser();

	void	tokenize(const std::string& filename);
	void	parse();
	void	parseServerBlock();
	void	parseListen(ServerConfig& server);

	class SyntaxException : public std::exception
	{
	public:
		virtual const char*	what() const throw();
	};
};



#endif