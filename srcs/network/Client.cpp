/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:33:49 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/16 20:23:23 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <sstream>

Client::Client(int fd, const ServerConfig& config) : _fd(fd), _config(config), _state(READING_REQUEST) {}

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

}
const HttpResponse&	Client::getResponse() const
{

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
			bool	keepAlive = true;
			std::string reqConnection = _request.getHeader("connection");
			if (reqConnection == "close")
				keepAlive = false;
		}
	}
	else if (bytesSend == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		std::cerr << "Client write error on FD " << this->_fd << std::endl;
		return false;
	}
	return false;
}

void	Client::prepareHttpResponse()
{
	
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
		if (questionMarkPos != std::string::npos)
		{
			cgiReq.scriptPath = "." + uri.substr(0, questionMarkPos);
			cgiReq.queryString = uri.substr(questionMarkPos + 1);
		}
		else
		{
			cgiReq.scriptPath = "." + uri;
			cgiReq.queryString = "";
		}

		cgiReq.httpBody = _request.getBody();
		cgiReq.headerInfo = _request.getHeaders();

		if (_cgi.initCgi(cgiReq) == false)
			return (false);
		return (true);
	}
	return (false);
}