/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 20:50:40 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/07 22:52:24 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

void	HttpRequest::trim(std::string& str)
{
	
}

HttpRequest::HttpRequest()
{
	clear();
}

HttpRequest::~HttpRequest() {}

void	HttpRequest::clear()
{
	_state = PARSE_REQUEST_LINE;
	_statusCode = 200;
	_rawBuffer.clear();

	_method = UNKNOWN;
	_uri.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_contentLength = 0;
	_isChunked = false;
}
HttpRequest::ParseState	HttpRequest::getState() const
{
	return	(_state);
}

int	HttpRequest::getStatusCode() const
{
	return (_statusCode);
}

HttpRequest::Method	HttpRequest::getMethod() const
{
	return (_method);
}

std::string	HttpRequest::getMethodStr() const
{
	switch ((_method))
	{
		case GET:		return "GET";
		case POST:		return "POST";
		case DELETE:	return "DELETE";
		default:		return "UNKNOWN";
	}
}

const std::string&	HttpRequest::getUri() const
{
	return (_uri);
}

const std::string&	HttpRequest::getVersion() const
{
	return (_version);
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const
{
	return (_headers);
}

std::string	HttpRequest::getHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end())
		return it->second;
	return "";
}

const std::string& HttpRequest::getBody() const
{
	return	(_body);
}