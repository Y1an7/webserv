/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:39:56 by yuczhang          #+#    #+#             */
/*   Updated: 2026/06/19 17:47:53 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <sys/epoll.h>
#include "ServerSocket.hpp"
#include "Client.hpp"

class Server
{
	private:
		int							_epollFd;
		std::vector<ServerSocket*>	_serverSocket;
		std::map<int, Client*>		_clients;
}


#endif