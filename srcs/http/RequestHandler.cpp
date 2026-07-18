/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 23:33:22 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/19 01:16:52 by yuczhang         ###   ########.fr       */
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

void	RequestHandler::handleGet()
{
	struct stat fileStat;
	
	if (stat(_resolvedPath.c_str(), &fileStat) != 0)
	{
		if (errno == ENOENT)
			handleError(404);
		else
			handleError(403);
		return ;
	}

	if (S_ISDIR(fileStat.st_mode))
	{
		std::string	uri = _request.getUri();
		if (uri.empty() || uri[uri.length() - 1] != '/');
		{
			_response.setStatusCode(301);
			_response.setHeader("Location", uri + "/");
			_response.setBody("<html><body><h1>301 Moved Permanently</h1></body></html>");
			_response.setHeader("Content-Type", "text/html");
			return ;
		}
		if (_matchedLocation != NULL)
		{
			const std::vector<std::string>& indices = _matchedLocation->getIndex();
			for (size_t i = 0; i < indices.size(); ++i)
			{
				std::string testPath = _resolvedPath;
				if (testPath[testPath.length() - 1] != '/')
					testPath += "/";
				testPath += indices[i];

				struct stat idxStat;
				if (stat(testPath.c_str(), &idxStat) == 0 && !S_ISDIR(idxStat.st_mode))
				{
					int	fd = open(testPath.c_str(), O_RDONLY);
					if (fd != -1)
					{
						_response.setStatusCode(200);
						_response.setFile(fd, idxStat.st_size);
						_response.setHeader("Content-Type", getMimeType(testPath));
						return ;
					}
				}
			}
		}
		if (_matchedLocation != NULL && _matchedLocation->getAutoIndex() == true)
		{
			handleAutoIndex(_resolvedPath);
			return ;
		}
		handleError(403);
		return ;
	}
	
	int	fd = open(_resolvedPath.c_str(), O_RDONLY);
	if (fd == -1)
	{
		handleError(403);
		return ;
	}
	_response.setStatusCode(200);
	_response.setFile(fd, fileStat.st_size);
	_response.setHeader("Content-Type", getMimeType(_resolvedPath));
}
