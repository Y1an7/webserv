/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/12 18:13:00 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/13 13:42:07 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include <string>
#include <cstdlib>
#include <algorithm>

bool	HttpRequest::parseRequestLine()
{
	std::string::size_type pos = _rawBuffer.find("\r\n");

	if (pos == std::string::npos)
	{
		if (_rawBuffer.length() > 8192)
		{
			_statusCode = 414; //URI too long
			_state = PARSE_ERROR;
		}
		return (false);
	}

	std::string requestLine = _rawBuffer.substr(0, pos);
	_rawBuffer.erase(0, pos + 2);
	if (requestLine.empty())
		return (true);

	std::string::size_type sp1 = requestLine.find(' ');
	std::string::size_type sp2 = requestLine.rfind(' ');
	if (sp1 == std::string::npos || sp1 == sp2 || requestLine.find(' ', sp1 + 1) != sp2)
	{
		_statusCode = 400;
		_state = PARSE_ERROR;
		return (false);
	}

	std::string methodStr = requestLine.substr(0, sp1);
	_uri = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
	if (_uri.empty())
	{
		_statusCode = 400;
		_state = PARSE_ERROR;
		return (false);
	}
	_version = requestLine.substr(sp2 + 1);
	if (methodStr == "GET")
		_method = GET;
	else if (methodStr == "POST")
		_method = POST;
	else if (methodStr == "DELETE")
		_method = DELETE;
	else
	{
		_method = UNKNOWN;
		_statusCode = 501;
		_state = PARSE_ERROR;
		return (false);
	}

	if (_version != "HTTP/1.1")
	{
		_statusCode = 505;
		_state = PARSE_ERROR;
		return (false);
	}
	
	_state = PARSE_HEADERS;
	return (true);
}

bool	HttpRequest::parseHeaders()
{
	std::string::size_type pos;

	if (_rawBuffer.length() > 8192 && _rawBuffer.find("\r\n") == std::string::npos)
	{
		_statusCode = 431;
		_state = PARSE_ERROR;
		return (false);
	}
	
	while ((pos = _rawBuffer.find("\r\n")) != std::string::npos)
	{
		std::string line = _rawBuffer.substr(0, pos);
		_rawBuffer.erase(0, pos + 2);
		
		if (line.empty())
		{
			if (_headers.find("host") == _headers.end())
			{
				_statusCode = 400;
				_state = PARSE_ERROR;
				return false;
			}
			
			if (_headers.count("transfer-encoding") && _headers["transfer-encoding"].find("chunked") != std::string::npos)
			{
				_isChunked = true;
				_state = PARSE_CHUNKED_BODY;
			}
			else if (_headers.count("content-length"))
			{
				std::string clValue = _headers["content-length"];
				for (size_t i = 0; i < clValue.length(); ++i)
				{
					if (!std::isdigit(static_cast<unsigned char>(clValue[i])))
					{
						_statusCode = 400;
						_state = PARSE_ERROR;
						return (false);
					}
				}
				_contentLength = std::strtol(_headers["content-length"].c_str(), NULL, 10);
				if (_contentLength > 0)
					_state = PARSE_BODY;
				else
					_state = PARSE_COMPLETE;
			}
			else
				_state = PARSE_COMPLETE;
			return (true);
		}
		
		std::string::size_type colonPos = line.find(':');
		if (colonPos == std::string::npos || colonPos == 0)
		{
			_statusCode = 400;
			_state = PARSE_ERROR;
			return (false);
		}

		if (line[colonPos - 1] == ' ' || line[colonPos - 1] == '\t')
		{
			_statusCode = 400;
			_state = PARSE_ERROR;
			return (false);
		}

		std::string key = line.substr(0, colonPos);
		std::string	value = line.substr(colonPos + 1);
		
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		trim(value);
		
		if (key == "content-length" && _headers.count("content-length"))
		{
			if (_headers["content-length"] != value)
			{
				_statusCode = 400;
				_state = PARSE_ERROR;
				return (false);
			}
		}
		
		if (_headers.find(key) == _headers.end())
			_headers[key] = value;
		else
			_headers[key] += ", " + value;
	}
	return (false);
}
