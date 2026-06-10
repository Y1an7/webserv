/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 16:50:43 by yuczhang          #+#    #+#             */
/*   Updated: 2026/06/10 20:07:03 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "incs/network_h/ServerSocket.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
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
		std::cout << "Server socket closed" << std::endl;
	}
}
void	ServerSocket::init()
{
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
		throw SocketException("Failed to create socket");
	
	int	opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw SocketException("Failed to set socket option SO_REUSEADDR");
	
	
}
