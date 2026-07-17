/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 16:27:40 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/16 20:40:07 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include <unistd.h>
#include <sstream>
#include <ctime>

HttpResponse::HttpResponse() : _fileFd(-1)
{
	this->reset();
}

HttpResponse::~HttpResponse()
{
	if (_fileFd != -1)
	{
		close(_fileFd);
		_fileFd = -1;
	}
}

HttpResponse::HttpResponse(const HttpResponse& other) : _fileFd(-1)
{
	*this = other;
}
HttpResponse& HttpResponse::operator=(const HttpResponse& other)
{
	if (this != &other)
	{
		_version = other._version;
		_statusCode = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers = other._headers;
		_body = other._body;
		_fileSize = other._fileSize;
		_headersSent = other._headersSent;
		_bytesSent = other._bytesSent;
		_headerString = other._headerString;

		if (_fileFd != -1)
		{
			close(_fileFd);
			_fileFd = -1;
		}
		if (other._fileFd != -1)
		{
			_fileFd = dup(other._fileFd);
			if (_fileFd == -1)
			{
				_statusCode = 500;
				_headers.clear();
				generateDefaultErrorPage();
				_headersSent = false;
				_bytesSent = 0;
				_fileSize = 0;
			}
		}
		else
			_fileFd = -1;
	}
	return (*this);
}

std::string	HttpResponse::getDefaultStatusMessage(int code) const
{
	switch (code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default:  return "Unknown Status";
	}
}

std::string	HttpResponse::intToString(size_t value) const
{
	std::stringstream	ss;
	ss << value;
	return ss.str();
}

std::string	HttpResponse::getCurrentDate() const
{
	char	buffer[128];
	time_t	now = time(NULL);
	
	struct	tm* timeinfo = gmtime(&now);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	
	return (std::string(buffer));
}

void	HttpResponse::generateDefaultErrorPage()
{
	if (_statusCode < 400)
	{
		_statusCode = 500;
		_statusMessage = "";
	}

	if (_statusMessage.empty())
		_statusMessage = getDefaultStatusMessage(_statusCode);
	
	std::string	codeStr = intToString(_statusCode);
	std::string	errorHtml = 
		"<html>\r\n"
		"<head><title>" + codeStr + " " + this->_statusMessage + "</title></head>\r\n"
		"<body style=\"font-family: Arial, sans-serif; text-align: center; margin-top: 10vh;\">\r\n"
		"	<h1 style=\"font-size: 4em; margin-bottom: 0;\">" + codeStr + "</h1>\r\n"
		"	<h2>" + this->_statusMessage + "</h2>\r\n"
		"	<hr style=\"width: 50%; max-width: 500px;\">\r\n"
		"	<p style=\"color: #4c4c4c;\">webserv/1.1</p>\r\n"
		"</body>\r\n"
		"</html>\r\n";
	setBody(errorHtml);
	setHeader("Content-Type", "text/html");
}

void	HttpResponse::reset()
{
	_version = "HTTP/1.1";
	_statusCode = 200;
	_statusMessage = "";
	_headers.clear();
	_body = "";
	
	if (_fileFd != -1)
		close(_fileFd);
	_fileFd = -1;
	_fileSize = 0;
	_headersSent = false;
	_bytesSent = 0;
	_headerString = "";
}

int	HttpResponse::getStatusCode() const
{
	return (_statusCode);
}

std::string	HttpResponse::getHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end())
		return (it->second);
	return ("");
}

int	HttpResponse::getFileFd() const
{
	return (_fileFd);
}

size_t	HttpResponse::getBytesSent() const
{
	return (_bytesSent);
}

void	HttpResponse::addBytesSent(size_t bytes)
{
	_bytesSent += bytes;
}

void	HttpResponse::setStatusCode(int code)
{
	_statusCode = code;
}

void	HttpResponse::setStatusMessage(const std::string& message)
{
	_statusMessage = message;
}

void	HttpResponse::setHeader(const std::string& key, const std::string& value)
{
	_headers[key] = value;
}

void	HttpResponse::setBody(const std::string& body)
{
	_body = body;
	setHeader("Content-Length", intToString(_body.length()));
}

void	HttpResponse::appendBody(const std::string& data)
{
	_body.append(data);
	setHeader("Content-Length", intToString(_body.length()));
}

void	HttpResponse::setFile(int fd, size_t size)
{
	if (_fileFd != -1 && _fileFd != fd)
		close (_fileFd);

	_fileFd = fd;
	_fileSize = size;
	setHeader("Content-Length", intToString(_body.length()));
}

const std::string& HttpResponse::buildAndGetHeaderString()
{
	if (getHeader("Server").empty())
		setHeader("Server", "webserv/1.1");
	if (getHeader("Data").empty())
		setHeader("Date", getCurrentDate());
	if (_statusMessage.empty())
		_statusMessage = getDefaultStatusMessage(_statusCode);
	
	std::stringstream	ss;
	ss << _version << " " << _statusCode << " " <<  _statusMessage << "\r\n";
	std::map<std::string, std::string>::const_iterator it;
	for (it = _headers.begin(); it != _headers.end(); ++it)
		ss << it->first << ": " << it->second << "\r\n";
	
	ss << "\r\n";
	_headerString = ss.str();
	return (_headerString);
}

bool	HttpResponse::isHeaderSent() const
{
	return (_headersSent);
}

void	HttpResponse::setHeaderSent(bool status)
{
	_headersSent = status;
}

bool	HttpResponse::isComplete() const
{
	if (!_headersSent)
		return false;
	
	size_t	expectedBodySize = 0;
	if (_fileFd != -1)
		expectedBodySize = _fileSize;
	else
		expectedBodySize = _body.length();
	
	return (_bytesSent >= expectedBodySize);
}
