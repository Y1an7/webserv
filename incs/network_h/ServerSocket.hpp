/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:40:30 by yuczhang          #+#    #+#             */
/*   Updated: 2026/06/09 19:21:39 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <exception>

class ServerSocket
{
	private:
		int					_fd;
		int					_port;
		std::string			_host;
		struct sockaddr_in	_adress;
	
		ServerSocket(const ServerSocket& other);
		ServerSocket&	operator=(const ServerSocket& other);
	public:
		ServerSocket(int port, const std::string& host = "0.0.0.0");
		~ServerSocket();

		void	init();
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