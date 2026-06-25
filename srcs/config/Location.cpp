#include "Location.hpp"

Location::Location()
{
	this->_path = "";
	this->_root = "";
	this->_autoindex = false;
	this->_cgiExtension = "";
	this->_cgiPath = "";
}

Location::Location(const Location& other)
{
	*this = other;
}

Location& Location::operator=(const Location& other)
{
	if (this != &other)
	{
		this->_path = other._path;
		this->_root = other._root;
		this->_index = other._index;
		this->_autoindex = other._autoindex;
		this->_methods = other._methods;
		this->_cgiExtension = other._cgiExtension;
		this->_cgiPath = other._cgiPath;
	}
	return *this;
}

Location::~Location() {}

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