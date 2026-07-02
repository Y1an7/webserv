/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 16:50:43 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/02 18:29:17 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "incs/network/ServerSocket.hpp"
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

ServerSocket::ServerSocket(const ServerConfig& config)
	:	_fd(-1), _config(config), _address() {}

ServerSocket::~ServerSocket()
{
	if (_fd != -1)
	{
		close(_fd);
		std::cout << "Server socket closed on port: " << _config.getPort() << std::endl;
	}
}
void	ServerSocket::init()
{
	std::stringstream ss;
	ss << _config.getPort();
	std::string portStr = ss.str();

	struct addrinfo hints = addrinfo();
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *res;

	if (getaddrinfo(_config.getHost().c_str(), portStr.c_str(), &hints, &res) != 0)
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
	{
		close(_fd);
		throw SocketException("Failed to bind socket to port");
	}
	if (listen(_fd, SOMAXCONN) < 0)
	{
		close(_fd);
		throw SocketException("Failed to listen on socket");
	}
}

int	ServerSocket::getFd() const
{
	return (_fd);
}

const ServerConfig& ServerSocket::getConfig() const
{
	return (_config);
}

int	ServerSocket::acceptConnect()
{
	struct sockaddr_in	client_addr = sockaddr_in();
	socklen_t			client_len = sizeof(client_addr);

	int client_fd = accept(_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
	if (client_fd < 0)
		return (-1);
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(client_fd);
		return (-1);
	}
	return (client_fd);
}
