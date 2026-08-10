/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rozhang <rozhang@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/15 23:33:01 by yuczhang          #+#    #+#             */
/*   Updated: 2026/08/10 21:31:54 by rozhang          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTHANDLER_HPP
#define	REQUESTHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include "Location.hpp"

class RequestHandler
{
	private:
		const HttpRequest&	_request;
		HttpResponse&		_response;
		const ServerConfig&	_config;
		const Location*		_matchedLocation;
		std::string			_resolvedPath;
	
		void		handleGet();
		void		handlePost();
		void		handleDelete();
		void		handleError(int statusCode);
		void		handleRedirect();

		void		handleCgi();
		bool		isCgi(const std::string& path) const;

		void		handleAutoIndex(const std::string& directoryPath);
	
		void		matchLocation();
		bool		isMethodAllowed() const;
		void		resolvePhysicalPath();

		bool		isDirectory(const std::string& path) const;
		bool		isFile(const std::string& path) const;

		std::string	getMimeType(const std::string& path) const;
		std::string	getExtension(const std::string& path) const;

		RequestHandler(const RequestHandler&);
		RequestHandler&	operator=(const RequestHandler&);
	
	public:
		RequestHandler(const HttpRequest& req, HttpResponse& res, const ServerConfig& config);
		~RequestHandler();

		void	execute();
};

#endif