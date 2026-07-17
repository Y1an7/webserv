/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 23:33:22 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/17 17:07:03 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

RequestHandler::RequestHandler(const HttpRequest& req, HttpResponse& res, const ServerConfig& config)
	: _request(req), _response(res), _config(config), _matchedLocation(NULL) {}

RequestHandler::~RequestHandler() {}

void	RequestHandler::execute()
{
	if (_request.getState() == HttpRequest::PARSE_ERROR)
	{
		handleError(_request.getStatusCode());
		return ;
	}
	
	matchLocation();
	if (!isMethodAllowed())
	{
		handleError(405);
		return ;
	}

	resolvePhysicalPath();
	switch (_request.getMethod())
	{
		case HttpRequest::GET:
			this->handleGet();
			break;
		case HttpRequest::POST:
			this->handlePost();
			break;
		case HttpRequest::DELETE:
			this->handleDelete();
			break;
		default:
			this->handleError(501); // Not Implemented
			break;
	}
}

