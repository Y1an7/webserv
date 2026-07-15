/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 20:50:40 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/15 21:04:39 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

void	HttpRequest::trim(std::string& str)
{
	std::string::size_type start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		str.clear();
		return ;
	}
	std::string::size_type end = str.find_last_not_of(" \t\r\n");
	str = str.substr(start, end - start + 1);
}

HttpRequest::HttpRequest()
{
	reset();
}

HttpRequest::~HttpRequest() {}

void	HttpRequest::feed(const std::string& data)
{
	if (_state == PARSE_ERROR)
		return ;
	_rawBuffer += data;
	bool keepParsing = true;

	while (keepParsing && _state != PARSE_COMPLETE && _state != PARSE_ERROR)
	{
		switch (_state)
		{
			case PARSE_REQUEST_LINE:
				keepParsing = parseRequestLine();
				break ;
			case PARSE_HEADERS:
				keepParsing = parseHeaders();
				if (keepParsing && (_state == PARSE_BODY || _state == PARSE_CHUNKED_BODY || _state == PARSE_COMPLETE))
					keepParsing = false;
				break ;
			case PARSE_BODY:
				keepParsing = parseNormalBody();
				break ;
			case PARSE_CHUNKED_BODY:
				keepParsing = parseChunkedBody();
				break ;
			default:
				keepParsing = false;
				break ;
		}
	}
}

void	HttpRequest::reset()
{
	_state = PARSE_REQUEST_LINE;
	_statusCode = 200;
	_method = UNKNOWN;
	_uri.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_maxBodySize = 1048576; //1MB
	_contentLength = 0;
	_isChunked = false;
	_chunkSize = -1;
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