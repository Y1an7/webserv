/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DummyConfig.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/14 18:50:31 by yuczhang          #+#    #+#             */
/*   Updated: 2026/06/14 23:58:10 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DUMMYCONFIG_HPP
#define DUMMYCONFIG_HPP

#include <string>

class DummyConfig
{
	public:
		int			port;
		std::string	host;
		size_t		max_client_body_size;
		
		DummyConfig() : _port(8080), _host("0.0.0.0"), max_client_body_size(1024 * 1024) {};
};

#endif