#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <iostream>
# include <string>
# include <vector>
# include <fstream>
# include "ServerConfig.hpp"


class ConfigParser
{
private:
	std::vector<std::string>	_tokens;
	size_t						_pos;

public:
	ConfigParser();
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);
	~ConfigParser();

	void	tokenize(const std::string& filename);
};



#endif