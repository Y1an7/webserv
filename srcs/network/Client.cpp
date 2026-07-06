/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:33:49 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/07 00:34:36 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <sstream>

Client::Client(int fd, const ServerConfig& config) : _fd(fd), _config(config), _state(READING_REQUEST) {}

Client::~Client() {}

int	Client::getFd() const
{
	return (this->_fd);
}

Client::State	Client::getState() const
{
	return (this->_state);
}

void	Client::setState(State state)
{
	this->_state = state;
}

bool	Client::readData()
{
	char	buffer[4096];
	int		bytesRead = recv(this->_fd, buffer, sizeof(buffer), 0);
	if (bytesRead > 0)
	{
		this->_requestBuffer.append(buffer, bytesRead);
		size_t	headerEnd = this->_requestBuffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos)
		{
			//httpRequest: request.iscComplete()
			this->_state = WRITING_RESPONSE;
		}
		return (true);
	}
	else if (bytesRead == 0)
		return (false);
	else
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (true);
		std::cerr << "Client read error on FD " << this->_fd << std::endl;
		return (false);
	}
}

bool	Client::writeData()
{
	if (this->_responseBuffer.empty())
		return true;
	int	bytesSend = send(this->_fd, this->_responseBuffer.c_str(), this->_responseBuffer.length(), 0);
	if (bytesSend > 0)
	{
		this->_responseBuffer.erase(0, bytesSend);
		if (this->_responseBuffer.empty())
			this->_state = CLOSE_CONNECTION;
		return true;
	}
	else if (bytesSend == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		std::cerr << "Client write error on FD " << this->_fd << std::endl;
		return false;
	}
	return false;
}

void	Client::prepareHttpResponse()
{
	
}