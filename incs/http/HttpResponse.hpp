/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:36:23 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/11 17:10:13 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <ctime>

class HttpResponse
{
	private:
		std::string							_version;
		int									_statusCode;
		std::string							_statusMessage;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		std::string							_rawResponse;
		bool								_isBuilt;

		std::string	getDefaultStatusMessage(int code) const;
		std::string	intToString(size_t value) const;
		std::string	getCurrentDate() const;
		void		generateDefaultErrorPage();
	public:
		HttpResponse();
		~HttpResponse();
		HttpResponse(const HttpResponse& other);
		HttpResponse& operator=(const HttpResponse& other);
		
		void	clear();
		void	setStatusCode(int code);
		void	setStatusMessage(const std::string& message);
		void	setHeader(const std::string& key, const std::string& value);
		void	setBody(const std::string& body);
		void	appendBody(const std::string& data);
		
		int					getStatusCode() const;
		std::string			getHeader(const std::string& key) const;
		const std::string&	getBody() const;

		void				build();
		const std::string&	toString();
		size_t				getLength() const;
};

#endif
