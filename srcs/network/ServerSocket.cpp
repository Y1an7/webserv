/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 16:50:43 by yuczhang          #+#    #+#             */
/*   Updated: 2026/06/12 19:38:41 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "incs/network/ServerSocket.hpp"
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

ServerSocket::ServerSocket(int port, const std::string& host)
	:	_fd(-1), _port(port), _host(host), _address() {}

ServerSocket::~ServerSocket()
{
	if (_fd != -1)
	{
		close(_fd);
		std::cout << "Server socket closed on port: " << _port << std::endl;
	}
}
void	ServerSocket::init()
{
	std::stringstream ss;
	ss << _port;
	std::string portStr = ss.str();
	struct addrinfo hints = addrinfo();
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *res;
	if (getaddrinfo(_host.c_str(), portStr.c_str(), &hints, &res) != 0)
		throw SocketException(std::string("Failed getaddrinfo"));
	
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
	{
		freeaddrinfo(res);
		throw SocketException("Failed to create socket");
	}
	
	int	opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		freeaddrinfo(res);
		close(_fd);
		throw SocketException("Failed to set socket option SO_REUSEADDR");
	}
	
	if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		freeaddrinfo(res);
		close(_fd);
		throw SocketException("Failed to set socket non-blocking mode");
	}
	
	_address = *reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
	freeaddrinfo(res);
	if (bind(_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
		throw SocketException("Failed to bind socket to port");
	if (listen(_fd, SOMAXCONN) < 0)
	{
		close(_fd);
		throw SocketException("Failed to listen on socket");
	}
}

int	ServerSocket::getFD() const
{
	return (_fd);
}

int	ServerSocket::acceptConnect()
{
	struct sockaddr_in	client_addr = sockaddr_in();
	socklen_t			client_len = sizeof(client_addr);

	int client_fd = accept(_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
	return (client_fd);
}
