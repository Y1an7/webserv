/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:33:49 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/23 00:57:18 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <sstream>

Client::Client(int fd, const ServerConfig& config) : _fd(fd), _config(config), _state(READING_REQUEST), _isCgiRequest(false)
{
	updateLastActivity();
	_request.setMaxBodySize(_config.getClientMaxBodySize());
}

Client::~Client() {}

int	Client::getFd() const
{
	return (this->_fd);
}

Client::State	Client::getState() const
{
	return (this->_state);
}

void	Client::setState(State state)
{
	this->_state = state;
}

const HttpRequest&	Client::getRequest() const
{
	return (_request);
}
const HttpResponse&	Client::getResponse() const
{
	return (_response);
}

bool	Client::readData()
{
	char	buffer[8192];
	ssize_t	bytesRead = recv(_fd, buffer, sizeof(buffer), 0);
	
	if (bytesRead > 0)
	{
		updateLastActivity();
		_request.feed(std::string(buffer, bytesRead));
		HttpRequest::ParseState reqState = _request.getState();
		if (reqState == HttpRequest::PARSE_COMPLETE)
		{
			if (checkAndInitCgi())
				_state = HANDLING_CGI;
			else
			{
				_state = WRITING_RESPONSE;
			}
		}
		else if (reqState == HttpRequest::PARSE_ERROR)
		{
			_state = WRITING_RESPONSE;
			prepareHttpResponse();
		}
		return (true);
	}
	else if (bytesRead == 0)
	{
		_state = CLOSE_CONNECTION;
		return (false);
	}
	else
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (true);
		std::cerr << "Error: Read error on fd " << _fd << ". Closing connection." << std::endl;
		_state = CLOSE_CONNECTION;
		return (false);
	}
}

bool	Client::writeData()
{
	if (_responseBuffer.empty())
		return (false);

	ssize_t	bytesSend = send(_fd, _responseBuffer.c_str(), _responseBuffer.length(), 0);
	if (bytesSend > 0)
	{
		updateLastActivity();
		_responseBuffer.erase(0, bytesSend);
		if (_responseBuffer.empty())
		{
			std::string reqConnection = _request.getHeader("connection");
			if (reqConnection == "close")
			{
				_state = CLOSE_CONNECTION;
				return (false);
			}
			_request.reset();
			_response.reset();
			_request.setMaxBodySize(_config.getClientMaxBodySize());
			_state = READING_REQUEST;
			updateLastActivity();
			return (true);
		}
		else
			return (true);
	}
	
	else if (bytesSend == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (true);
		std::cerr << "Client write error on FD " << this->_fd << std::endl;
		_state = CLOSE_CONNECTION;
		return (false);
	}
	_state = CLOSE_CONNECTION;
	return false;
}

void	Client::prepareHttpResponse()
{
	if (_isCgiRequest)
	{
		std::string cgiOutPut = _cgi.getOutput();
		if (cgiOutPut.empty())
		{
			std::string body = "<html><body><h1>500 CGI Execution Failed</h1></body></html>";
			std::stringstream ss;
			ss << "HTTP/1.1 500 Internal Server Error\r\n"
				<< "Content-Type: text/html\r\n"
				<< "Content-Length: " << body.length() << "\r\n\r\n"
				<< body;
			_responseBuffer = ss.str();
		}
		else
		{
			size_t headerEnd = cgiOutPut.find("\r\n\r\n");
			if (headerEnd != std::string::npos)
			{
				std::string cgiHeaders = cgiOutPut.substr(0, headerEnd);
				std::string cgiBody = cgiOutPut.substr(headerEnd + 4);
				
				std::stringstream ss;
				ss << "HTTP/1.1 200 OK\r\n"
					<< cgiHeaders << "\r\n"
					<< "Content-Length: " << cgiBody.length() << "\r\n\r\n"
					<< cgiBody;
				_responseBuffer = ss.str();
			}
			else
				_responseBuffer = "HTTP/1.1 200 OK\r\n\r\n" + cgiOutPut;
		}
		return ;
	}
	RequestHandler handler(_request, _response, _config);
	handler.execute();

	std::string headerStr = _response.buildAndGetHeaderString();
	_responseBuffer = headerStr;

	if (_response.getFileFd() == -1 && _response.getStatusCode() != 204)
		_responseBuffer += _response.getBody();
}


CgiHandler& Client::getCgiHandler()
{
	return (_cgi);
}


bool	Client::checkAndInitCgi()
{
	std::string uri = _request.getUri();
	if (uri.find(".py") != std::string::npos || uri.find(".php") != std::string::npos)
	{
		_isCgiRequest = true;
		CgiRequest	cgiReq;
		cgiReq.method = _request.getMethodStr();

		size_t	questionMarkPos = uri.find('?');
		std::string pathOnly;
		if (questionMarkPos != std::string::npos)
		{
			pathOnly = uri.substr(0, questionMarkPos);
			cgiReq.queryString = uri.substr(questionMarkPos + 1);
		}
		else
		{
			pathOnly = uri;
			cgiReq.queryString = "";
		}

		std::string rootDir = _config.getRoot();
		if (rootDir.empty())
			rootDir = "./www";
		
		if (!rootDir.empty() && rootDir[rootDir.length() - 1] == '/')
			rootDir.erase(rootDir.length() - 1);
		if (!pathOnly.empty() && pathOnly[0] != '/')
			pathOnly = "/" + pathOnly;

		cgiReq.scriptPath = rootDir + pathOnly;

		cgiReq.httpBody = _request.getBody();
		cgiReq.headerInfo = _request.getHeaders();

		if (_cgi.initCgi(cgiReq) == false)
			return (false);
		return (true);
	}
	return (false);
}

void	Client::updateLastActivity()
{
	_lastActivity = time(NULL);
}

bool	Client::hasTimedOut(time_t timeoutSeconds) const
{
	time_t	currentTime = time(NULL);
	return ((currentTime - _lastActivity) > timeoutSeconds);
}

void	Client::handleTimeout()
{
	if (_state == READING_REQUEST)
	{
		_response.setStatusCode(408);
		_response.generateDefaultErrorPage();
	}
	else if (_state == HANDLING_CGI)
	{
		_cgi.killCgi();
		_response.setStatusCode(504);
		_response.generateDefaultErrorPage();
	}
	else
	{
		_state = CLOSE_CONNECTION;
		return ;
	}
	
	_response.generateDefaultErrorPage();
	_responseBuffer = _response.buildAndGetHeaderString() + _response.getBody();
	_state = WRITING_RESPONSE;
}
