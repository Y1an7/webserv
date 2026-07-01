/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:33:53 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/01 22:49:32 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "ServerSocket.hpp"

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cerrno>

Server::Server() :_epollFd(-1)
{
	_epollFd = epoll_create(10);
	if (_epollFd == -1)
		throw EpollException(std::string("Failed to create epoll instance: ") + strerror(errno));
}

Server::~Server()
{
	if (_epollFd != -1)
		close(_epollFd);
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		delete it->second;
	_clients.clear();
	for (size_t i = 0; i < _serverSocket.size(); ++i)
		delete _serverSocket[i];
	_serverSocket.clear();
}

void	Server::addServerSocket(ServerSocket* server)
{
	for (size_t i = 0; i < _serverSocket.size(); ++i)
	{
		int	serverFd = _serverSocket[i]->getFd();
		
		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN;
		ev.data.fd = serverFd;

		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, serverFd, &ev) == -1)
			throw EpollException(std::string("epoll_ctl ADD serverFd failed: ") + strerror(errno));
		std::cout << "Server successfully initialized epoll with " << _serverSocket.size() << " ports." << std::endl;
	}
}


