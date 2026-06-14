/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:40:30 by yuczhang          #+#    #+#             */
/*   Updated: 2026/06/15 00:03:51 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

#include <netinet/in.h>
#include <string>
#include <exception>
#include "DummyConfig.hpp"

class ServerSocket
{
	private:
		int					_fd;
		DummyConfig			_config;
		struct sockaddr_in	_address;
	
		ServerSocket(const ServerSocket& other);
		ServerSocket&	operator=(const ServerSocket& other);
	public:
		ServerSocket(const DummyConfig& config);
		~ServerSocket();

		void	init();
		int		acceptConnect();
		int		getFD() const;

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