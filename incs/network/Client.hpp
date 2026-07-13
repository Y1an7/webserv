/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:39:24 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/13 18:23:30 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>

class Client
{
	public:
		enum State
		{
			READING_REQUEST,
			WRITING_RESPONSE,
			CLOSE_CONNECTION
		};
	
	private:
		int					_fd;
		const ServerConfig&	_config;	
		HttpRequest			_request;
		HttpResponse		_response;
		State				_state;
		
		Client(const Client& other);
		Client&	operator=(const Client& other);
		
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

		void				prepareHttpResponse();
};

#endif