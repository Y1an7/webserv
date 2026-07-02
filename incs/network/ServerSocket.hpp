/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:40:30 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/02 18:52:26 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

#include <netinet/in.h>
#include <string>
#include <exception>
#include "incs/config/ServerConfig.hpp"

class ServerSocket
{
	private:
		int					_fd;
		ServerConfig		_config;
		struct sockaddr_in	_address;
	
		ServerSocket(const ServerSocket& other);
		ServerSocket&	operator=(const ServerSocket& other);
	public:
		ServerSocket(const ServerConfig& config);
		~ServerSocket();

		void				init();
		int					acceptConnect();
		int					getFd() const;
		const ServerConfig& getConfig() const;

		class SocketException : public std::exception
		{
			private:
				std::string _msg;
			public:
				SocketException(const std::string& msg) : _msg(msg) {}
				virtual ~SocketException() throw() {}
				virtual const char* what() const throw() { return _msg.c_str(); }
		};
};

#endif