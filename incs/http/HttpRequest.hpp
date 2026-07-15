/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 20:50:37 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/15 21:03:57 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>

class HttpRequest
{
	public:
		enum Method { GET, POST, DELETE, UNKNOWN };
		enum ParseState
		{
			PARSE_REQUEST_LINE,
			PARSE_HEADERS,
			PARSE_BODY,
			PARSE_CHUNKED_BODY,
			PARSE_COMPLETE,
			PARSE_ERROR
		};
	private:
		ParseState							_state;
		int									_statusCode;
		std::string							_rawBuffer;
		
		Method								_method;
		std::string							_uri;
		std::string							_version;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		size_t								_maxBodySize;
		size_t								_contentLength;
		bool								_isChunked;
		long								_chunkSize;

		bool	parseRequestLine();
		bool	parseHeaders();
		bool	parseNormalBody();
		bool	parseChunkedBody();

		void	trim(std::string& str);
	
	public:
		HttpRequest();
		~HttpRequest();
		
		void	feed(const std::string& data); //core interface
		void	reset(); // reset
		
		ParseState									getState() const;
		int											getStatusCode() const;
		Method										getMethod() const;
		std::string									getMethodStr() const;
		const std::string& 							getUri() const;
		const std::string&							getVersion() const;
		const std::map<std::string, std::string>&	getHeaders() const;
		std::string									getHeader(const std::string& key) const;
		const std::string&							getBody() const;
};

#endif