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
	std::string	result = "";
	char		buffer[4096];
	int			bytes_read;
	int			status;

	_buildEnvp(req);
	_buildArgv(req);

	if (pipe(_pipe_in) == -1 || pipe(_pipe_out) == -1)
	{
		_freeArray(_envp);
		_envp = NULL;
		_freeArray(_argv);
		_argv = NULL;
		return "Status: 500 Internal Server Error\r\n\r\n";
	}
	
	_pid = fork();
	if (_pid == -1)
	{
		_freeArray(_envp);
		_envp = NULL;
		_freeArray(_argv);
		_argv = NULL;
		return "Status: 500 Internal Server Error\r\n\r\n";
	}

	if (_pid == 0)
	{
		dup2(_pipe_in[0], STDIN_FILENO);
		dup2(_pipe_out[1], STDOUT_FILENO);

		close(_pipe_in[0]);
		close(_pipe_in[1]);
		close(_pipe_out[0]);
		close(_pipe_out[1]);

		execve(_argv[0], _argv, _envp);

		std::cerr << "CGI execve failed!" << std::endl;
		_freeArray(_envp);
		_freeArray(_argv);
		exit(1);
	}

	else
	{
		close(_pipe_in[0]);
		_pipe_in[0] = -1;
		close(_pipe_out[1]);
		_pipe_out[1] = -1;

		if (!req.httpBody.empty())
			write(_pipe_in[1], req.httpBody.c_str(), req.httpBody.length());

		close(_pipe_in[1]);
		_pipe_in[1] = -1;

		waitpid(_pid, &status, 0);

		bytes_read = read(_pipe_out[0], buffer, sizeof(buffer) - 1);
		while (bytes_read > 0)
		{
			buffer[bytes_read] = '\0';
			result += buffer;
			bytes_read = read(_pipe_out[0], buffer, sizeof(buffer) - 1);
		}

		close(_pipe_out[0]);
		_pipe_out[0] = -1;

		_freeArray(_envp);
		_envp = NULL;
		_freeArray(_argv);
		_argv = NULL;
	}
	return result;
}