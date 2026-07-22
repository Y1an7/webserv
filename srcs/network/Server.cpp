/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:33:53 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/22 23:07:43 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "ServerSocket.hpp"
#include "CgiHandler.hpp"

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

void Server::addServerSocket(ServerSocket* server)
{
	if (server)
		_serverSocket.push_back(server);
}

void	Server::initEpoll()
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
	}
	std::cout << "Server successfully initialized epoll with " << _serverSocket.size() << " ports." << std::endl;
}

void	Server::run()
{
	std::cout << "Starting server main event loop..." << std::endl;
	
	while (true)
	{
		int	numEvents = epoll_wait(_epollFd, _events, MAX_EVENTS, 1000);
		if (numEvents == -1)
		{
			if (errno == EINTR)
				continue ;
			throw EpollException(std::string("epoll_wait failed: ") + strerror(errno));
		}

		for (int i = 0; i < numEvents; ++i)
		{
			int 		triggeredFd = _events[i].data.fd;
			uint32_t	events = _events[i].events;

			
			//case 1: CGI fd handling
			if (_cgiReadFds.find(triggeredFd) != _cgiReadFds.end())
			{
				if (events & (EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP))
					handleCgiRead(triggeredFd);
				continue ;
			}
			if (_cgiWriteFds.find(triggeredFd) != _cgiWriteFds.end())
			{
				if (events & (EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP))
					handleCgiWrite(triggeredFd);
				continue ;
			}

			//case 2: fatal network socket errors (client only)
			if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				if (_clients.find(triggeredFd) != _clients.end())
					removeClient(triggeredFd);
				continue ;
			}

			//case 3: if there is new connection
			bool			isNewConnection = false;
			ServerSocket*	matchedServer = NULL;
			
			for (size_t j = 0; j < _serverSocket.size(); ++j)
			{
				if (triggeredFd == _serverSocket[j]->getFd())
				{
					isNewConnection = true;
					matchedServer = _serverSocket[j];
					break ;
				}
			}
			if (isNewConnection)
				acceptNewClient(matchedServer);

			//case 4: client read/write
			else
			{
				if (events & EPOLLIN)
					handleClientRead(triggeredFd);
				if ((events & EPOLLOUT) && _clients.find(triggeredFd) != _clients.end())
					handleClientWrite(triggeredFd);
			}
		}

		for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			Client* client = it->second;
			if (client->getState() == Client::HANDLING_CGI)
			{
				if (client->getCgiHandler().checkTimeout(5))
				{
					client->setState(Client::WRITING_RESPONSE);
					client->prepareHttpResponse();
					struct epoll_event ev;
					std::memset(&ev, 0, sizeof(ev));
					ev.events = EPOLLOUT | EPOLLRDHUP;
					ev.data.fd = client->getFd();
					epoll_ctl(_epollFd, EPOLL_CTL_MOD, client->getFd(), &ev);
				}
			}
		}
	}
}

void	Server::acceptNewClient(ServerSocket* server)
{
	int	clientFd = server->acceptConnect();
	if (clientFd == -1)
	{
		std::cerr << "Failed to accept new client connection." << std ::endl;
		return ;
	}
	
	struct epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN | EPOLLRDHUP;
	ev.data.fd = clientFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1)
	{
		std::cerr << "Failed to add client to epoll: " << strerror(errno) << std::endl;
		close(clientFd);
		return ;
	}
	
	Client* newClient = new Client(clientFd, server->getConfig());
	_clients[clientFd] = newClient;
	std::cout << "New client accepted on FD: " << clientFd << std::endl;
}

void	Server::handleClientRead(int clientFd)
{
	if (_clients.find(clientFd) == _clients.end())
		return ;
	
	Client* client = _clients[clientFd];
	if (!client->readData())
	{
		removeClient(clientFd);
		return ;
	}
	
	if (client->getState() == Client::HANDLING_CGI)
		registerCgiFds(client);
	else if (client->getState() == Client::WRITING_RESPONSE)
	{
		client->prepareHttpResponse();
		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLOUT | EPOLLRDHUP;
		ev.data.fd = clientFd;
		
		if (epoll_ctl(_epollFd,EPOLL_CTL_MOD, clientFd, &ev) == -1)
		{
			removeClient(clientFd);
			return ;
		}
	}
}

void	Server::handleClientWrite(int clientFd)
{
	if (_clients.find(clientFd) == _clients.end())
		return ;
	
	Client* client = _clients[clientFd];
	if (!client->writeData())
	{
		removeClient(clientFd);
		return ;
	}
	
	if (client->getState() == Client::READING_REQUEST)
	{
		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN | EPOLLRDHUP;
		ev.data.fd = clientFd;

		if (epoll_ctl(_epollFd,EPOLL_CTL_MOD, clientFd, &ev) == -1)
			removeClient(clientFd);
	}
	else if (client->getState() == Client::CLOSE_CONNECTION)
	{
		removeClient(clientFd);
		return ;
	}
}


void Server::registerCgiFds(Client* client)
{
    CgiHandler& cgi = client->getCgiHandler();
    int readFd = cgi.getReadFd();
    int writeFd = cgi.getWriteFd();


    if (readFd != -1)
    {
        struct epoll_event evIn;
        std::memset(&evIn, 0, sizeof(evIn));
        evIn.events = EPOLLIN | EPOLLRDHUP;
        evIn.data.fd = readFd;
        
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, readFd, &evIn) == -1)
            std::cerr << "Epoll ADD readFd FAILED!" << std::endl;
        else
            std::cout << "Successfully registered readFd: " << readFd << " to Epoll" << std::endl;
        
        _cgiReadFds[readFd] = client;
    }

    if (writeFd != -1)
    {
        if (cgi.getState() == CgiHandler::CGI_WRITING)
        {
            struct epoll_event evOut;
            std::memset(&evOut, 0, sizeof(evOut));
            evOut.events = EPOLLOUT | EPOLLRDHUP;
            evOut.data.fd = writeFd;
            
            if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, writeFd, &evOut) == -1)
                std::cerr << "Epoll ADD writeFd FAILED!" << std::endl;
            else
                std::cout << "Successfully registered writeFd: " << writeFd << " to Epoll (EPOLLOUT)" << std::endl;
            
            _cgiWriteFds[writeFd] = client;
        }
        else
        {
            std::cout << "ERROR: writeFd ignored because CGI state is NOT CGI_WRITING" << std::endl;
        }
    }
}

void	Server::removeClient(int clientFd)
{
	std::cout << "Disconnecting client FD: " << clientFd << std::endl;
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
	close(clientFd);

	std::map<int, Client*>::iterator it = _clients.find(clientFd);
	if (it != _clients.end())
	{
		Client* client = it->second;

		int readFd = client->getCgiHandler().getReadFd();
		int writeFd = client->getCgiHandler().getWriteFd();

		if (readFd != -1)
		{
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, readFd, NULL);
			_cgiReadFds.erase(readFd);
		}
		if (writeFd != -1)
		{
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, writeFd, NULL);
			_cgiWriteFds.erase(writeFd);
		}
		client->getCgiHandler().killCgi();

		delete it->second;
		_clients.erase(it);
	}
}


void	Server::handleCgiWrite(int fd)
{
	Client* client = _cgiWriteFds[fd];
	CgiHandler& cgi = client->getCgiHandler();

	std::cout << "handleCgiWrite triggered for FD: " << fd << std::endl;

	if (cgi.writeToCgi() == false)
	{
			std::cout << "writeToCgi returned false!" << std::endl;		{
			cleanupCgiFds(fd, false);
			client->setState(Client::WRITING_RESPONSE);
			client->prepareHttpResponse();
		}
	}

	if (cgi.getState() == CgiHandler::CGI_READING)
		cleanupCgiFds(fd, false);
}


void	Server::handleCgiRead(int fd)
{
	Client* client = _cgiReadFds[fd];
	CgiHandler& cgi = client->getCgiHandler();

	if (cgi.readFromCgi() == false && cgi.getState() == CgiHandler::CGI_ERROR)
	{
		cleanupCgiFds(fd, true);
		client->setState(Client::WRITING_RESPONSE);
		client->prepareHttpResponse();
		return ;
	}

	if (cgi.getState() == CgiHandler::CGI_DONE)
	{
		cleanupCgiFds(fd, true);
		client->setState(Client::WRITING_RESPONSE);
		client->prepareHttpResponse();

		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLOUT | EPOLLRDHUP;
		ev.data.fd = client->getFd();
		epoll_ctl(_epollFd, EPOLL_CTL_MOD, client->getFd(), &ev);
	}
}


void	Server::cleanupCgiFds(int fd, bool isReadFd)
{
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
	if (isReadFd)
		_cgiReadFds.erase(fd);
	else
		_cgiWriteFds.erase(fd);
}