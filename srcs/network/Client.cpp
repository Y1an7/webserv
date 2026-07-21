/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:33:49 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/21 20:30:14 by rozhang          ###   ########.fr       */
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

Client::Client(int fd, const ServerConfig& config) : _fd(fd), _config(config), _state(READING_REQUEST), _isCgiRequest(false) {}

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
		_request.feed(std::string(buffer, bytesRead));
		HttpRequest::ParseState reqState = _request.getState();
		if (reqState == HttpRequest::PARSE_COMPLETE)
		{
			if (checkAndInitCgi())
				_state = HANDLING_CGI;
			else
			{
				_state = WRITING_RESPONSE;
				prepareHttpResponse();
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
			_state = READING_REQUEST;
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
			_responseBuffer = "HTTP/1.1 500 Internal Server Error\r\n"
			                  "Content-Type: text/html\r\n"
			                  "Content-Length: 53\r\n\r\n"
			                  "<html><body><h1>500 CGI Execution Failed</h1></body></html>";
		}
		else
			_responseBuffer = "HTTP/1.1 200 OK\r\n" + cgiOutPut;
		return ;
	}
	RequestHandler handler(_request, _response, _config);
	handler.execute();

	std::string headerStr = _response.buildAndGetHeaderString();
	_responseBuffer = headerStr;

	if (_response.getFileFd() == -1)
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