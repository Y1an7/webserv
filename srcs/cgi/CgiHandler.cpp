#include "CgiHandler.hpp"
#include <cctype>
#include <sstream>
#include <sys/wait.h>
#include <cstdlib>

CgiHandler::CgiHandler() : _envp(NULL), _argv(NULL), _pid(-1)
{
	_pipe_in[0] = -1;
	_pipe_in[1] = -1;
	_pipe_out[0] = -1;
	_pipe_out[1] = -1;
}

CgiHandler::CgiHandler(const CgiHandler& other)
{
	*this = other;
}

CgiHandler& CgiHandler::operator=(const CgiHandler& other)
{
	if (this != &other)
	{
		_pid = other._pid;
		_pipe_in[0] = other._pipe_in[0];
		_pipe_in[1] = other._pipe_in[1];
		_pipe_out[0] = other._pipe_out[0];
		_pipe_out[1] = other._pipe_out[1];
	}
	return *this;
}

CgiHandler::~CgiHandler()
{
	_freeArray(_envp);
	_freeArray(_argv);

	if (_pipe_in[0] != -1) 
		close(_pipe_in[0]);
	if (_pipe_in[1] != -1) 
		close(_pipe_in[1]);
	if (_pipe_out[0] != -1) 
		close(_pipe_out[0]);
	if (_pipe_out[1] != -1) 
		close(_pipe_out[1]);
}

void	CgiHandler::_freeArray(char ** array)
{
	int i;

	if (!array)
		return ;
	i = 0;
	while (array[i] != NULL)
	{
		delete[] array[i];
		i++;
	}
	delete[] array;
}


std::string	formattedCgiHeaderKey(const std::string& originalKey)
{
	std::string	formattedKey = originalKey;
	std::string::iterator str_it = formattedKey.begin();

	while (str_it != formattedKey.end())
	{
		if (*str_it == '-')
			*str_it = '_';
		else
			*str_it = std::toupper(*str_it);
		++str_it;
	}
	return formattedKey;
}



void	CgiHandler::_buildArgv(const CgiRequest& req)
{
	_argv = new char*[2]; //allocate for 2 string pointers

	_argv[0] = new char[req.scriptPath.length() + 1];

	size_t i = 0;
	while (i < req.scriptPath.length())
	{
		_argv[0][i] = req.scriptPath[i];
		i++;
	}
	_argv[0][i] = '\0';

	_argv[1] = NULL; //mandatory array terminator
}



void	CgiHandler::_buildEnvp(const CgiRequest& req)
{
	std::vector<std::string>	envVars;

	envVars.push_back("REQUEST_METHOD=" + req.method);
	envVars.push_back("QUERY_STRING=" + req.queryString);
	envVars.push_back("SCRIPT_FILENAME=" + req.scriptPath);

	if (!req.httpBody.empty())
	{
		std::stringstream ss;
		ss << req.httpBody.length();
		envVars.push_back("CONTENT_LENGTH=" + ss.str());
	}
	//if there is a length, the cgi script needs to know its length

	std::map<std::string, std::string>::const_iterator it = req.headerInfo.begin();
	
	while (it != req.headerInfo.end())
	{
		std::string	cgiKey = "HTTP_" + formattedCgiHeaderKey(it->first);
		envVars.push_back(cgiKey + "=" + it->second);
		++it;
	}
	//it->first grabs the Map's Key, it->second grabs the Map's Value

	_envp = new char*[envVars.size() + 1];

	size_t i = 0;
	while (i < envVars.size())
	{
		_envp[i] = new char[envVars[i].length() + 1];
		size_t j = 0;
		while (j < envVars[i].length())
		{
			_envp[i][j] = envVars[i][j];
			j++;
		}
		_envp[i][j] = '\0';
		i++;
	}
	_envp[i] = NULL;
}

std::string CgiHandler::execute(const CgiRequest& req)
{
	
}