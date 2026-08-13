/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:39:24 by yuczhang          #+#    #+#             */
/*   Updated: 2026/08/13 18:49:17 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include <string>
#include <ctime>

class Client
{
	public:
		enum State
		{
			READING_REQUEST,
			WRITING_RESPONSE,
			CLOSE_CONNECTION,
			HANDLING_CGI
		};
	
	private:
		int							_fd;
		std::vector<ServerConfig>   _configs;
		const ServerConfig*			_activeConfig;
		HttpRequest					_request;
		HttpResponse				_response;
		std::string					_responseBuffer;
		size_t						_sendOffset;
		State						_state;
		
		CgiHandler			_cgi;
		bool				_isCgiRequest;
		time_t				_lastActivity;
				
		Client(const Client& other);
		Client&	operator=(const Client& other);
	
	public:
		Client(int fd, const std::vector<ServerConfig>& configs);
		~Client();

		int					getFd() const;
		State				getState() const;
		void				setState(State state);
		const HttpRequest&	getRequest() const;
		const HttpResponse&	getResponse() const;

		void				resolveActiveConfig();
		void				setResponseBuffer(const std::string& data);

		bool				readData();
		bool				writeData();

		CgiHandler&			getCgiHandler();
		bool				checkAndInitCgi();

		void				prepareHttpResponse();

		void				updateLastActivity();
		bool				hasTimedOut(time_t timeoutSeconds) const;
		void				handleTimeout();
};

#endif