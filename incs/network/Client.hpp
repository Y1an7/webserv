/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:39:24 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/22 18:46:29 by yuczhang         ###   ########.fr       */
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
		int					_fd;
		const ServerConfig&	_config;	
		HttpRequest			_request;
		HttpResponse		_response;
		std::string			_responseBuffer;
		State				_state;
		
		Client(const Client& other);
		Client&	operator=(const Client& other);
		
		CgiHandler			_cgi;
		bool				_isCgiRequest;

		time_t				_lastActivity;

	public:
		Client(int fd, const ServerConfig& config);
		~Client();

		int					getFd() const;
		State				getState() const;
		void				setState(State state);
		const HttpRequest&	getRequest() const;
		const HttpResponse&	getResponse() const;

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