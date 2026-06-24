#include "Location.hpp"

void	Location::setPath(const std::string& path)
{
	this->_path = path;
}

void	Location::addIndex(const std::string& indexPage)
{
	this->_index.push_back(indexPage);
}

void	Location::setRoot(const std::string& root)
{
	this->_root = root;
}

void	Location::setAutoindex(bool autoIndex)
{
	this->_autoindex = autoIndex;
}

void	Location::addMethod(const std::string& method)
{
	this->_methods.push_back(method);
}

void	Location::setCgiPath(const std:: string& cgiPath)
{
	this->_cgiPath = cgiPath;
}

void	Location::setCgiExtension(const std::string& cgiExt)
{
	this->_cgiExtension = cgiExt;
}