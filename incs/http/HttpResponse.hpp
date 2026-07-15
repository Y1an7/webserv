/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:36:23 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/15 22:19:26 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse
{
	private:
		std::string							_version;
		int									_statusCode;
		std::string							_statusMessage;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		int									_fileFd;
		size_t								_fileSize;
		bool								_headersSent;
		size_t								_bytesSent;
		std::string							_headerString;

		std::string	getDefaultStatusMessage(int code) const;
		std::string	intToString(size_t value) const;
		std::string	getCurrentDate() const;
		void		generateDefaultErrorPage();

	public:
		HttpResponse();
		~HttpResponse();
		HttpResponse(const HttpResponse& other);
		HttpResponse& operator=(const HttpResponse& other);
		
		void	reset();

		int					getStatusCode() const;
		std::string			getHeader(const std::string& key) const;
		int					getFileFd() const;
		const std::string&	getBody() const;
		size_t				getBytesSent() const;
		void				addBytesSent(size_t bytes);
		
		void				setStatusCode(int code);
		void				setStatusMessage(const std::string& message);
		void				setHeader(const std::string& key, const std::string& value);
		void				setBody(const std::string& body);
		void				appendBody(const std::string& data);
		void				setFile(int fd, size_t size);
		
		const std::string&	buildAndGetHeaderString();
		bool				isHeaderSent() const;
		void				setHeaderSent(bool status);
	
		bool				isComplete() const;

};

#endif
