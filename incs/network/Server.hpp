/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:39:56 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/22 22:04:32 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <exception>
#include <map>
#include <string>
#include <sys/epoll.h>
#include <vector>

class ServerSocket;
class Client;

class Server
{
	private:
		int							_epollFd;
		std::vector<ServerSocket*>	_serverSocket;
		std::map<int, Client*>		_clients;
		static const int			MAX_EVENTS = 1024;
		struct epoll_event			_events[MAX_EVENTS];

		std::map<int, Client*> _cgiReadFds;
		std::map<int, Client*> _cgiWriteFds;

		void	registerCgiFds(Client* client);
		void	handleCgiRead(int fd);
		void	handleCgiWrite(int fd);
		void	cleanupCgiFds(int fd, bool isReadFd);

		void	acceptNewClient(ServerSocket* server);
		void	handleClientRead(int clientFd);
		void	handleClientWrite(int clientFd);
		void	removeClient(int clientFd);
	
	public:
		Server();
		~Server();
		void	addServerSocket(ServerSocket* server);
		void	initEpoll();
		void	run();

		class EpollException : public std::exception
		{
			private:
				std::string _msg;
			public:
				EpollException(std::string msg) : _msg(msg) {}
				virtual	~EpollException() throw() {}
				virtual const char* what() const throw() { return (_msg.c_str()); }
		};
};

#endif