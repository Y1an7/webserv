/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/12 18:13:00 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/12 20:27:58 by yuczhang         ###   ########.fr       */
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
}
