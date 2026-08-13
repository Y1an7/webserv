/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yuczhang <yuczhang@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 23:33:22 by yuczhang          #+#    #+#             */
/*   Updated: 2026/08/13 13:38:38 by yuczhang         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "ServerConfig.hpp"
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <fstream>
#include <sstream>


RequestHandler::RequestHandler(const HttpRequest& req, HttpResponse& res, const ServerConfig& config)
	: _request(req), _response(res), _config(config), _matchedLocation(NULL), _pathRejected(false) {}

RequestHandler::~RequestHandler() {}

void	RequestHandler::handleRedirect()
{
	int 		code = _matchedLocation->getRedirectCode();
	std::string	url = _matchedLocation->getRedirectUrl();

	_response.setStatusCode(code);
	_response.setHeader("Location", url);
	_response.setHeader("Content-Type", "text/html");
	_response.setBody("<html>...The document has moved <a href=\"" + url + "\">here</a>...</html>");
	if (_request.getMethod() == HttpRequest::HEAD)
		_response.discardBodyForHead();
}

void	RequestHandler::execute()
{
	if (_request.getState() == HttpRequest::PARSE_ERROR)
	{
		handleError(_request.getStatusCode());
		return ;
	}
	
	else
	{
		matchLocation();
		if (_matchedLocation != NULL && _matchedLocation->hasClientMaxBodySize())
		{
			if (_request.getBody().length() > _matchedLocation->getClientMaxBodySize())
			{
				handleError(413);
				if (_request.getMethod() == HttpRequest::HEAD)
					_response.discardBodyForHead();
				return ;
			}
		}
		if (_matchedLocation != NULL && _matchedLocation->getRedirectCode() != 0)
		{
			handleRedirect();
			return ;
		}
		if (!isMethodAllowed())
		{
			std::string allow;
			if (_matchedLocation != NULL)
			{
				const std::vector<std::string>& m =_matchedLocation->getMethods();
				for (size_t i = 0; i < m.size(); ++i)
					allow += (i ? ", " : "") + m[i];
			}
			_response.setHeader("Allow", allow.empty() ? "GET, HEAD" : allow);
			handleError(405);
			return ;
		}

		else
		{
			resolvePhysicalPath();
			switch (_request.getMethod())
			{
				case HttpRequest::GET:
					this->handleGet();
					break;
				case HttpRequest::HEAD:
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
	}
	if (_request.getMethod() == HttpRequest::HEAD)
		_response.discardBodyForHead();
}

void	RequestHandler::handleGet()
{
	if (_resolvedPath.empty() || _pathRejected)
	{
		handleError(403);
		if (_request.getMethod() == HttpRequest::HEAD)
			_response.discardBodyForHead();
		return ;
	}
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
		

		std::string uri = _request.getUri();
		
		if (!_resolvedPath.empty() && _resolvedPath[_resolvedPath.length() - 1] != '/')
		{
			_resolvedPath += '/';
		}
		
		std::vector<std::string> indices; //location level index first, then server level index
		if (_matchedLocation != NULL)
			indices = _matchedLocation->getIndex();
		if (indices.empty())
			indices = _config.getIndex();
		{
			for (size_t i = 0; i < indices.size(); ++i)
			{
				std::string testPath = _resolvedPath + indices[i];

				struct stat idxStat;
				if (stat(testPath.c_str(), &idxStat) == 0 && !S_ISDIR(idxStat.st_mode))
				{
					std::ifstream file(testPath.c_str(), std::ios::in | std::ios::binary);
					
					if (file.is_open())
					{
						std::ostringstream  ss;
						ss << file.rdbuf();

						_response.setStatusCode(200);
						_response.setHeader("Content-Type", getMimeType(testPath));

						_response.setBody(ss.str());
						file.close();
						if (_request.getMethod() == HttpRequest::HEAD)
							_response.discardBodyForHead();
						return ;
					}
				}
			}
		}
		if (_matchedLocation != NULL && _matchedLocation->getAutoIndex() == true)
		{
			handleAutoIndex(_resolvedPath);
			if (_request.getMethod() == HttpRequest::HEAD)
				_response.discardBodyForHead();
			return ;
		}
		handleError(404);
		if (_request.getMethod() == HttpRequest::HEAD)
			_response.discardBodyForHead();
		return ;
	}
	
	int	fd = open(_resolvedPath.c_str(), O_RDONLY);
	if (fd == -1)
	{
		handleError(403);
		if (_request.getMethod() == HttpRequest::HEAD)
			_response.discardBodyForHead();
		return ;
	}
	_response.setStatusCode(200);
	_response.setFile(fd, fileStat.st_size);
	_response.setHeader("Content-Type", getMimeType(_resolvedPath));

	std::ostringstream ssFallback;
	ssFallback << fileStat.st_size;
	_response.setHeader("Content-Length", ssFallback.str());
	if (_request.getMethod() == HttpRequest::HEAD)
	{
		close(fd);
		return ;
	}
}

void	RequestHandler::matchLocation()
{
	_matchedLocation = _config.matchLocation(_request.getUri());
}

bool	RequestHandler::isMethodAllowed() const
{
	if (_matchedLocation == NULL)
		return (_request.getMethod() == HttpRequest::GET
				|| _request.getMethod() == HttpRequest::HEAD);
	
	const std::vector<std::string>& allowedMethods = _matchedLocation->getMethods();
	if (allowedMethods.empty())
		return (true);
	
	std::string currentMethodStr;
	HttpRequest::Method reqMethod = _request.getMethod();

	if (reqMethod == HttpRequest::GET)
		currentMethodStr = "GET";
	else if (reqMethod == HttpRequest::HEAD)
		currentMethodStr = "HEAD";
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
std::string RequestHandler::percentDecode(const std::string& s) const
{
	std::string result;
	result.reserve(s.length());
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (s[i] == '%' && i + 2 < s.length())
		{
			std::string hexStr = s.substr(i + 1, 2);
			char* endPtr = NULL;
			long val = std::strtol(hexStr.c_str(), &endPtr, 16);
			
			if (endPtr == hexStr.c_str() + 2)
			{
				result.push_back(static_cast<char>(val));
				i += 2;
			}
			else
			{
				result.push_back(s[i]);
			}
		}
		else
		{
			result.push_back(s[i]);
		}
	}
	return result;
}

bool RequestHandler::isPathSafe(const std::string& uriPath) const
{
	std::istringstream ss(uriPath);
	std::string segment;
	
	// 按照 '/' 分割路径，如果发现任何一个目录层级是 ".."，立刻判定为不安全
	while (std::getline(ss, segment, '/'))
	{
		if (segment == "..")
			return false;
	}
	return true;
}

void RequestHandler::resolvePhysicalPath()
{
	std::string rootPath;
	bool hasLocationRoot = false;

	if (_matchedLocation != NULL && !_matchedLocation->getRoot().empty())
	{
		rootPath = _matchedLocation->getRoot();
		hasLocationRoot = true;
	}
	else
		rootPath = _config.getRoot();

	while (!rootPath.empty() && rootPath[rootPath.length() - 1] == '/')
		rootPath.erase(rootPath.length() - 1);

	std::string	pathOnly = _request.getUri();
	size_t q = pathOnly.find('?');
	if (q != std::string::npos)
		pathOnly.erase(q);

	pathOnly = percentDecode(pathOnly);

	if (!isPathSafe(pathOnly))
	{
		_resolvedPath.clear();
		_pathRejected = true;
		return ;
	}

	std::string leftoverUri = pathOnly;
    if (_matchedLocation != NULL && hasLocationRoot)
    {
        const std::string& locPath = _matchedLocation->getPath();
        if (leftoverUri.compare(0, locPath.length(), locPath) == 0)
            leftoverUri.erase(0, locPath.length());
    }
    
    if (leftoverUri.empty() || leftoverUri[0] != '/')
        leftoverUri = "/" + leftoverUri;

    _resolvedPath = rootPath + leftoverUri;

    char resolved[PATH_MAX];
    char rootReal[PATH_MAX];
    
    if (realpath(rootPath.c_str(), rootReal) != NULL)
    {
        if (realpath(_resolvedPath.c_str(), resolved) != NULL)
        {
            std::string realPathStr(resolved);
            std::string rootRealStr(rootReal);
            
            if (realPathStr.compare(0, rootRealStr.length(), rootRealStr) != 0)
            {
                _resolvedPath.clear();
                _pathRejected = true;
                return ;
            }
        }
    }
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
		if (ext == ".txt" || ext == ".bad_extension")
			return "text/plain";
	}
	return "application/octet-stream";
}

void RequestHandler::handlePost()
{
	if (_resolvedPath.empty() || _pathRejected)
	{
		handleError(403);
		return ;
	}
	
	struct stat fileStat;

	if (_matchedLocation != NULL && !_matchedLocation->getUploadStore().empty())
	{
		std::string store = _matchedLocation->getUploadStore();
		while (!store.empty() && store[store.length() - 1] == '/')
			store.erase(store.length() - 1);
		
		std::string uri = _request.getUri();
		size_t q = uri.find('?');
		if (q != std::string::npos)
			uri.erase(q);
		std::string base = uri.substr(uri.find_last_of('/') + 1);
		if (base.empty() || base == "." || base == ".." || base.find('/') != std::string::npos)
		{
			handleError(400);
			return ;
		}

		struct stat storeStat;
		if (stat(store.c_str(), &storeStat) != 0 || !S_ISDIR(storeStat.st_mode))
		{
			handleError(500);
			return ;
		}
		_resolvedPath = store + "/" + base;
	}
	
	std::string createdUri = _request.getUri();
	if (stat(_resolvedPath.c_str(), &fileStat) == 0)
	{
		if (S_ISDIR(fileStat.st_mode))
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			std::stringstream ss;
			ss << tv.tv_sec << "_" << tv.tv_usec;
			std::string uniqueName = "/upload_" + ss.str();
			_resolvedPath += uniqueName;
			if (createdUri.length() > 0 && createdUri[createdUri.length() - 1] != '/')
				createdUri += "/";
			createdUri += "upload_" + ss.str();
		}
		else if (S_ISREG(fileStat.st_mode) &&
					(_matchedLocation == NULL || _matchedLocation->getUploadStore().empty()))
		{
			handleError(409);
			return ;
		}
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
	_response.setHeader("Location", createdUri);
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
	if (_resolvedPath.empty() || _pathRejected)
	{
		handleError(403);
		return ;
	}
	
	struct stat fileStat;

	//verify existence and initial access permissions
	if (stat(_resolvedPath.c_str(), &fileStat) != 0)
	{
		if (errno == ENOENT)
			handleError(404);
		else
			handleError(403);
		return ;
	}

	//prevent directory deletion (unlinked is for files only)
	if (S_ISDIR(fileStat.st_mode))
	{
		handleError(403);
		return ;
	}

	//execute system deletion
	if (unlink(_resolvedPath.c_str()) == 0)
	{
		_response.setStatusCode(204);
		_response.setHeader("Content-Length", "0");
		_response.setBody("");
	}
	else
	{
		if (errno == EACCES || errno == EPERM)
			handleError(403);
		else
			handleError(500);
	}
}

void RequestHandler::handleError(int statusCode)
{
	_response.setStatusCode(statusCode);
	std::string errorPagePath = _config.getErrorPages(statusCode);
	
	bool	customErrorPage = false;

	if (_request.getMethod() != HttpRequest::HEAD)
	{
		if (!errorPagePath.empty())
		{
			std::string root = _config.getRoot();
			while (!root.empty() && root[root.length() - 1] == '/')
				root.erase(root.length() - 1);
			if (errorPagePath[0] != '/')
				errorPagePath = "/" + errorPagePath;
			std::string fullPath = root + errorPagePath;
			
			std::ifstream file(fullPath.c_str(), std::ios::binary);
			if (file.is_open())
			{
                std::ostringstream ss;
                ss << file.rdbuf();

                _response.setBody(ss.str());
                _response.setHeader("Content-Type", getMimeType(fullPath));

                std::stringstream lenStr;
                lenStr << ss.str().length();
                _response.setHeader("Content-Length", lenStr.str());

				customErrorPage = true;
					return ;
			}
		}
	}
	if (!customErrorPage)
		_response.generateDefaultErrorPage();
	if (_request.getMethod() == HttpRequest::HEAD)
		_response.discardBodyForHead();
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