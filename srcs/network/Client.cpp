/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:33:49 by yuczhang          #+#    #+#             */
/*   Updated: 2026/08/13 18:52:07 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

Client::Client(int fd, const std::vector<ServerConfig>& configs) : _fd(fd), _configs(configs), _sendOffset(0), _state(READING_REQUEST), _isCgiRequest(false)
{
	updateLastActivity();
	if (!_configs.empty())
	{
		_request.setMaxBodySize(_configs[0].getClientMaxBodySize());
		_activeConfig = &_configs[0];
	}
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

void	Client::resolveActiveConfig()
{
	if (_configs.empty())
		return ;
	std::string hostHeader = _request.getHeader("Host");
	size_t	colonPos = hostHeader.find(':');
	if (colonPos != std::string::npos)
		hostHeader = hostHeader.substr(0, colonPos);

	_activeConfig = &(_configs[0]);
	bool foundMatch = false;
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		const std::vector<std::string>& names = _configs[i].getServerNames();
		for (size_t j = 0; j < names.size(); ++j)
		{
			if (names[j] == hostHeader)
			{
				_activeConfig = &(_configs[i]);
				foundMatch = true;
				break ;
			}
		}
		if (foundMatch)
			break ;
	}
	_request.setMaxBodySize(_activeConfig->getClientMaxBodySize());
}

void	Client::setResponseBuffer(const std::string& data)
{
	_responseBuffer = data;
	_sendOffset = 0;
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
			resolveActiveConfig();
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
		std::cerr << "Error: Read error on fd " << _fd << ". Closing connection." << std::endl;
		_state = CLOSE_CONNECTION;
		return (false);
	}
}

bool	Client::writeData()
{
	if (_sendOffset >= _responseBuffer.length())
	{
		_responseBuffer.clear();
		_sendOffset = 0;

		int fd = _response.getFileFd();
		if (fd != -1)
		{
			char buf[8192]; //8kb
			ssize_t bytesRead = read(fd, buf, sizeof(buf));

			if (bytesRead > 0)
				_responseBuffer.append(buf, bytesRead);
			else if (bytesRead == 0)
			{
				close(fd);
				_response.setFile(-1, 0);
			}
			else
			{
				std::cerr << "Error reading file FD: " << fd << std::endl;
				close(fd);
				_response.setFile(-1, 0);
				_state = CLOSE_CONNECTION;
				return (false);
			}
				
		}
	}

	if (_sendOffset >= _responseBuffer.length())
	{
		_responseBuffer.clear();
		_sendOffset = 0;

		std::string reqConnection = _request.getHeader("Connection");
		if (reqConnection == "close")
		{
			_state = CLOSE_CONNECTION;
			return (false);
		}

		std::string conn = _request.getHeader("Connection");
		for (size_t i = 0; i < conn.length(); ++i)
			conn[i] = static_cast<char>(::tolower(static_cast<unsigned char>(conn[i])));
		if (_response.mustClose() || conn.find("close") != std::string::npos
			 || _request.getState() == HttpRequest::PARSE_ERROR)
		{
			_state = CLOSE_CONNECTION;
			return (false);
		}

		_request.reset();
		_response.reset();
		_isCgiRequest = false;
		_cgi.killCgi();
		_cgi.reset();
		_request.setMaxBodySize(_activeConfig->getClientMaxBodySize());
		_state = READING_REQUEST;
		updateLastActivity();
		return (true);
	}

	if (_sendOffset > _responseBuffer.length())
		_sendOffset = 0;
	size_t chunk = _responseBuffer.length() - _sendOffset;
	if (chunk > 65536)
		chunk = 65536;
	ssize_t bytesSend = send(_fd, _responseBuffer.c_str() + _sendOffset, chunk, 0);
	if (bytesSend > 0)
	{
		updateLastActivity();
		_sendOffset += static_cast<size_t>(bytesSend);
		if (_sendOffset >= _responseBuffer.length())
		{
			_responseBuffer.clear();
			_sendOffset = 0;
		}
		return (true);
	}

	else if (bytesSend == -1)
	{
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
			setResponseBuffer(ss.str());
			return ;
		}
		else
		{
			const std::string& cgiOutPut = _cgi.getOutput();
			size_t headerEnd = cgiOutPut.find("\r\n\r\n");
			size_t splitLen = 4;
			if (headerEnd == std::string::npos)
			{
				headerEnd = cgiOutPut.find("\n\n");
				splitLen = 2;
			}

			if (headerEnd != std::string::npos)
			{
				std::string cgiHeaders = cgiOutPut.substr(0, headerEnd);
				
				int statusCode = 200;
				std::string statusMsg = "OK";
				std::string remainingHeaders;
				std::istringstream headerStream(cgiHeaders);
				std::string line;

				while (std::getline(headerStream, line))
				{
					if (!line.empty() && line[line.length() - 1] == '\r')
						line.erase(line.length() - 1);
						
					if (line.compare(0, 8, "Status: ") == 0)
					{
						std::istringstream statusLine(line.substr(8));
						statusLine >> statusCode;
						std::getline(statusLine, statusMsg);
						if (!statusMsg.empty() && statusMsg[0] == ' ')
							statusMsg.erase(0, 1);
					}
					else
					{
						remainingHeaders += line + "\r\n";
					}
				}

				std::stringstream ss;
				ss << "HTTP/1.1 " << statusCode << " " << statusMsg << "\r\n"
					<< remainingHeaders
					<< "Content-Length: " << (cgiOutPut.length() - headerEnd - splitLen) << "\r\n\r\n";

				setResponseBuffer(ss.str());
				_responseBuffer.append(cgiOutPut.c_str() + headerEnd + splitLen, cgiOutPut.length() - headerEnd - splitLen);
			}
			else
			{
				setResponseBuffer("HTTP/1.1 200 OK\r\n\r\n");
				_responseBuffer.append(cgiOutPut);
			}
		}
		_cgi.clearOutput();
		return ;
	}
    
	RequestHandler handler(_request, _response, *_activeConfig);
	handler.execute();

	std::string headerStr = _response.buildAndGetHeaderString();
	setResponseBuffer(headerStr);

	if (_response.getFileFd() == -1 && _response.getStatusCode() != 204)
		_responseBuffer += _response.getBody();
}


CgiHandler& Client::getCgiHandler()
{
	return (_cgi);
}

bool    Client::checkAndInitCgi()
{
    _isCgiRequest = false;
    
    std::string uri = _request.getUri();
    std::string pathOnly;
    std::string queryString;
    
    size_t  questionMarkPos = uri.find('?');
    if (questionMarkPos != std::string::npos)
    {
        pathOnly = uri.substr(0, questionMarkPos);
        queryString = uri.substr(questionMarkPos + 1);
    }
    else
    {
        pathOnly = uri;
        queryString = "";
    }

    const Location* matchedLoc = _activeConfig->matchLocation(pathOnly);

    if (!matchedLoc || matchedLoc->getCgiExtension().empty())
        return (false);

    std::string configuredCgiExt = matchedLoc->getCgiExtension();

    if (pathOnly.length() >= configuredCgiExt.length() && 
        pathOnly.compare(pathOnly.length() - configuredCgiExt.length(), configuredCgiExt.length(), configuredCgiExt) == 0)
    {
		const std::vector<std::string>& allowedMethods = matchedLoc->getMethods();
		if (!allowedMethods.empty())
		{
			bool methodAllowed = false;
			std::string reqMethod = _request.getMethodStr();
			for (size_t i = 0; i < allowedMethods.size(); ++i)
			{
				if (allowedMethods[i] == reqMethod)
				{
					methodAllowed = true;
					break ;
				}
			}
			if (!methodAllowed)
				return (false);
		}
        _isCgiRequest = true;
        CgiRequest  cgiReq;
        cgiReq.method = _request.getMethodStr();
        cgiReq.queryString = queryString;
		cgiReq.uri = pathOnly;
		std::string rootDir;
		bool hasLocationRoot = false;

        if (!matchedLoc->getRoot().empty())
		{
            rootDir = matchedLoc->getRoot();
			hasLocationRoot = true;
		}
        else
            rootDir = _activeConfig->getRoot();
            
        if (rootDir.empty())
            rootDir = "./www";
        
        while (!rootDir.empty() && rootDir[rootDir.length() - 1] == '/')
            rootDir.erase(rootDir.length() - 1);
		std::string leftoverUri = pathOnly;
		if (hasLocationRoot)
		{
			const std::string& locPath = matchedLoc->getPath();
			if (leftoverUri.compare(0, locPath.length(), locPath) == 0)
				leftoverUri.erase(0, locPath.length());
		}

		if (leftoverUri.empty() || leftoverUri[0] != '/')
			leftoverUri = "/" + leftoverUri;

		cgiReq.scriptPath = rootDir + leftoverUri;

        while (!cgiReq.scriptPath.empty() &&
            (cgiReq.scriptPath[cgiReq.scriptPath.length() - 1] == '\r' || 
             cgiReq.scriptPath[cgiReq.scriptPath.length() - 1] == '\n' || 
             cgiReq.scriptPath[cgiReq.scriptPath.length() - 1] == ' '))
        {
            cgiReq.scriptPath.erase(cgiReq.scriptPath.length() - 1);
        }
		
		cgiReq.executorPath = matchedLoc->getCgiPath();
        std::string& reqBody = const_cast<std::string&>(_request.getBody());
		cgiReq.httpBody.swap(reqBody);
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
	setResponseBuffer(_response.buildAndGetHeaderString() + _response.getBody());
	_state = WRITING_RESPONSE;
}
