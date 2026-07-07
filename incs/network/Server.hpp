/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:39:56 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/07 19:59:29 by yuczhang         ###   ########.fr       */
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