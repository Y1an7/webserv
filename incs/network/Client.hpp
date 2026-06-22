/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 22:39:24 by yuczhang          #+#    #+#             */
/*   Updated: 2026/06/17 22:09:17 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "DummyConfig.hpp"
#include <string>

class Client
{
	public:
		enum State
		{
			READING_REQUEST,
			WRITING_RESPONSE,
			CLOSE_CONNECTION
		};
	
	private:
		int					_fd;
		const DummyConfig&	_config;	
		std::string			_requestBuffer;
		std::string			_responseBuffer;
		State				_state;
		
		Client(const Client& other);
		Client&	operator=(const Client& other);
		
	public:
		Client(int fd, const DummyConfig& config);
		~Client();

		int		getFD() const;
		State	getState() const;
		void	setState(State state);

		bool	readData();
		bool	writeData();

		void	prepareHttpResponse();
};

#endif