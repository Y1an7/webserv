/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 23:33:22 by yuczhang          #+#    #+#             */
/*   Updated: 2026/07/21 11:13:00 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "ServerConfig.hpp"
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
		if (uri.empty() || uri[uri.length() - 1] != '/')
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

void	RequestHandler::matchLocation()
{
	const std::string& uri = _request.getUri();
	const std::vector<Location>& locations = _config.getLocations();

	_matchedLocation = NULL;
	size_t	maxMatchLength = 0;
	
	for (size_t i = 0; i < locations.size(); ++i)
	{
		const std::string& locPath = locations[i].getPath();
		if (uri.find(locPath) == 0)
		{
			bool isDirectoryMatch = (uri.length() == locPath.length()) || 
									(!locPath.empty() && locPath[locPath.length() - 1] == '/') ||
									(uri[locPath.length()] == '/');
			if (isDirectoryMatch)
			{
				if (locPath.length() > maxMatchLength)
				{
					maxMatchLength = locPath.length();
					_matchedLocation = &locations[i];
				}
			}
		}
	}
}

bool	RequestHandler::isMethodAllowed() const
{
	if (_matchedLocation == NULL)
		return (_request.getMethod() == HttpRequest::GET);
	const std::vector<std::string>& allowedMethods = _matchedLocation->getMethods();
	if (allowedMethods.empty())
		return (true);
	
	std::string currentMethodStr;
	HttpRequest::Method reqMethod = _request.getMethod();
	if (reqMethod == HttpRequest::GET)
		currentMethodStr = "GET";
	else if (reqMethod == HttpRequest::POST)
		currentMethodStr = "POST";
	else if (reqMethod == HttpRequest::DELETE)
		currentMethodStr = "DELETE";
	else
		return false;
	for (size_t i = 0; i < allowedMethods.size(); ++i)
	{
		if (allowedMethods[i] == currentMethodStr)
			return (true);
	}
	return (false);
}

void RequestHandler::resolvePhysicalPath()
{
	std::string rootPath;

	if (_matchedLocation != NULL && !_matchedLocation->getRoot().empty())
		rootPath = _matchedLocation->getRoot();
	else
		rootPath = "./www";
	if (!rootPath.empty() && rootPath[rootPath.length() - 1] == '/')
		rootPath.erase(rootPath.length() - 1);
	_resolvedPath = rootPath + _request.getUri();
}

std::string	RequestHandler::getMimeType(const std::string& path) const
{
	size_t dotPos = path.find_last_of('.');
	if (dotPos != std::string::npos)
	{
		std::string ext = path.substr(dotPos);
		if (ext == ".html" || ext == ".htm")
			return "text/html";
		if (ext == ".css")
			return "text/css";
		if (ext == ".js")
			return "application/javascript";
		if (ext == ".png")
			return "image/png";
		if (ext == ".jpg" || ext == ".jpeg")
			return "image/jpeg";
		if (ext == ".gif")
			return "image/gif";
		if (ext == ".txt")
			return "text/plain";
	}
	return "application/octet-stream";
}

void RequestHandler::handlePost()
{
	struct stat fileStat;
	bool isDirectory = false;

	if (stat(_resolvedPath.c_str(), &fileStat) == 0)
	{
		if (S_ISDIR(fileStat.st_mode))
			isDirectory = true;
	}

	if (isDirectory)
	{
		handleError(403);
		return ;
	}

	int fd = open(_resolvedPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1)
	{
		handleError(403);
		return ;
	}

	const std::string& body = _request.getBody();
	ssize_t byteWritten = write(fd, body.c_str(), body.length());

	close(fd);

	if (byteWritten == -1 || static_cast<size_t>(byteWritten) != body.length())
	{
		handleError(500);
		return ;
	}

	_response.setStatusCode(201);
	_response.setHeader("Content-Type", "text/html");
	_response.setHeader("Location", _request.getUri());
	_response.setBody(
			"<html>\r\n"
			"<head><title>201 Created</title></head>\r\n"
			"<body>\r\n"
			"<h1>201 Created</h1>\r\n"
			"<p>The file was successfully created/uploaded.</p>\r\n"
			"</body>\r\n"
			"</html>\r\n"
	);
}

void RequestHandler::handleDelete()
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
		handleError(403);
		return ;
	}

	if (unlink(_resolvedPath.c_str()) == 0)
	{
		_response.setStatusCode(204);
		_response.setHeader("Content-Length", "0");
	}
	else
		handleError(403);
}

void RequestHandler::handleError(int statusCode)
{
	_response.setStatusCode(statusCode);
	std::string errorPagePath = _config.getErrorPages(statusCode);

	if (!errorPagePath.empty())
	{
		struct stat fileState;
		if (stat(errorPagePath.c_str(), &fileState) == 0 && !S_ISDIR(fileState.st_mode))
		{
			int fd = open(errorPagePath.c_str(), O_RDONLY);
			if (fd != -1)
			{
				_response.setFile(fd, fileState.st_size);
				_response.setHeader("Content-Type", getMimeType(errorPagePath));
				return ;
			}
		}
	}
	_response.generateDefaultErrorPage();
}

void RequestHandler::handleAutoIndex(const std::string& path)
{
	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
	{
		handleError(403);
		return ;
	}

	std::string uri = _request.getUri();
	std::string html =
			"<html>\r\n"
			"<head><title>Index of " + uri + "</title></head>\r\n"
			"<body style=\"font-family: Arial, sans-serif;\">\r\n"
			"<h1>Index of " + uri + "</h1>\r\n"
			"<hr>\r\n"
			"<pre>\r\n";
	
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string filename = entry->d_name;

		if (filename == ".")
			continue ;
		
		std::string fullPath = path;
		if (fullPath[fullPath.length() - 1] != '/')
			fullPath += "/";
		fullPath += filename;

		struct stat fileStat;
		bool isDir = false;
		if (stat(fullPath.c_str(), &fileStat) == 0)
		{
			if (S_ISDIR(fileStat.st_mode))
				isDir = true;
		}

		std::string displayName = filename;
		if (isDir)
			displayName += "/";

		html += "<a href=\"" + displayName + "\">" + displayName + "</a>\r\n";
	}
	closedir(dir);
	html += "</pre>\r\n<hr>\r\n</body>\r\n</html>\r\n";

	_response.setStatusCode(200);
	_response.setHeader("Content-Type", "text/html");
	_response.setBody(html);
}