#include "Location.hpp"

Location::Location()
{
	this->_path = "";
	this->_root = "";
	this->_autoindex = false;
	this->_cgiExtension = "";
	this->_cgiPath = "";
	this->_clientMaxBodySize = 0;
	this->_hasClientMaxBodySize = false;
	this->_redirectCode = 0;
	this->_redirectUrl = "";
	this->_uploadStore = "";
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
		this->_clientMaxBodySize = other._clientMaxBodySize;
		this->_hasClientMaxBodySize = other._hasClientMaxBodySize;
		this->_redirectCode = other._redirectCode;
		this->_redirectUrl = other._redirectUrl;
		this->_uploadStore = other._uploadStore;
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

void	Location::setClientMaxBodySize(size_t size)
{
	this->_clientMaxBodySize = size;
	this->_hasClientMaxBodySize = true;
}

void Location::setRedirect(int code, const std::string& uri)
{
	_redirectCode = code;
	_redirectUrl = uri;
}

void Location::setUploadStore(const std::string path)
{
	_uploadStore = path;
}

const std::string& Location::getPath() const
{
	return this->_path;
}

const std::vector<std::string>& Location::getIndex() const
{
	return this->_index;
}

const std::string& Location::getRoot() const
{
	return this->_root;
}

bool Location::getAutoIndex() const
{
	return this->_autoindex;
}

const std::vector<std::string>& Location::getMethods() const
{
	return this->_methods;
}

const std::string& Location::getCgiExtension() const
{
	return this->_cgiExtension;
}

const std::string& Location::getCgiPath() const
{
	return this->_cgiPath;
}

size_t Location::getClientMaxBodySize() const
{
	return this->_clientMaxBodySize;
}

bool Location::hasClientMaxBodySize() const
{
	return this->_hasClientMaxBodySize;
}

int	Location::getRedirectCode() const
{
	return this->_redirectCode;
}

const std::string& Location::getRedirectUrl() const
{
	return this->_redirectUrl;
}

const std::string& Location::getUploadStore() const
{
	return this->_uploadStore;
}