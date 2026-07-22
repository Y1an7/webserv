#include "CgiHandler.hpp"
#include <fcntl.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <iostream>
#include <stdlib.h>
#include <errno.h>

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
		_state = other._state;
		_inputBuffer = other._inputBuffer;
		_outputBuffer = other._outputBuffer;
		_startTime = other._startTime;
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

	if (_pid > 0)
		killCgi();
}

void	CgiHandler::_freeArray(char ** array)
{
	if (!array)
		return ;
	int i = 0;
	while (array[i] != NULL)
	{
		delete[] array[i];
		i++;
	}
	delete[] array;
}

void	CgiHandler::_setNonBlocking(int fd)
{
	if (fd != -1)
		fcntl(fd, F_SETFL, O_NONBLOCK);
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


void	CgiHandler::_buildEnvp(const CgiRequest& req)
{
	std::vector<std::string>	envVars;

	envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
	envVars.push_back("SERVER_SOFTWARE=webserv/1.1");
	envVars.push_back("REQUEST_METHOD=" + req.method);
	envVars.push_back("QUERY_STRING=" + req.queryString);
	envVars.push_back("SCRIPT_FILENAME=" + req.scriptPath);
	envVars.push_back("REDIRECT_STATUS=200");

	if (req.headerInfo.find("Content-Length") != req.headerInfo.end())
		envVars.push_back("CONTENT_LENGTH=" + req.headerInfo.at("Content-Length"));
	else if (!req.httpBody.empty())
	{
		std::stringstream ss;
		ss << req.httpBody.length();
		envVars.push_back("CONTENT_LENGTH=" + ss.str());
	}

	if (req.headerInfo.find("Content-Type") != req.headerInfo.end())
		envVars.push_back("CONTENT-TYPE=" + req.headerInfo.at("Content-Type"));

	std::map<std::string, std::string>::const_iterator it = req.headerInfo.begin();
	
	while (it != req.headerInfo.end())
	{
		std::string key = it->first;
		if (key != "Content-Length" && key != "Content-Type")
		{
			std::string	cgiKey = "HTTP_" + formattedCgiHeaderKey(it->first);
			envVars.push_back(cgiKey + "=" + it->second);
		}
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




bool CgiHandler::initCgi(const CgiRequest& req)
{
	_buildEnvp(req);
	_buildArgv(req);
	_inputBuffer = req.httpBody;
	_outputBuffer = "";

	if (pipe(_pipe_in) == -1)
	{
		_state = CGI_ERROR;
		return false;
	} 
	
	if (pipe(_pipe_out) == -1)
	{
		close(_pipe_in[0]);
		close(_pipe_in[1]);
		_state = CGI_ERROR;
		return false;
	}

	//non-blocking for parent's endpoints
	_setNonBlocking(_pipe_in[1]);
	_setNonBlocking(_pipe_out[0]);
	
	gettimeofday(&_startTime, NULL);

	_pid = fork();
	if (_pid == -1)
	{
		close(_pipe_in[0]); close(_pipe_in[1]);
		close(_pipe_out[0]); close(_pipe_out[1]);
		_freeArray(_envp); _envp = NULL;
		_freeArray(_argv); _argv = NULL;
		_state = CGI_ERROR;
		return false;
	}

	if (_pid == 0)
	{
		dup2(_pipe_in[0], STDIN_FILENO);
		dup2(_pipe_out[1], STDOUT_FILENO);

		close(_pipe_in[0]); close(_pipe_in[1]);
		close(_pipe_out[0]); close(_pipe_out[1]);

		execve(_argv[0], _argv, _envp);

		std::cerr << "[CGI Error] execve failed for " << _argv[0]
				<< ": " << strerror(errno) << std::endl;
		_freeArray(_envp);
		_freeArray(_argv);
		_exit(1);
	}

	else
	{
		close(_pipe_in[0]); _pipe_in[0] = -1;
		close(_pipe_out[1]); _pipe_out[1] = -1;

		_freeArray(_envp); _envp = NULL;
		_freeArray(_argv); _argv = NULL;

		if (_inputBuffer.empty())
		{
			close(_pipe_in[1]);
			_pipe_in[1] = -1;
			_state = CGI_READING;
		}
		else
			_state = CGI_WRITING;
	}
	return true;
}



bool CgiHandler::writeToCgi()
{
	if (_pipe_in[1] == -1)
		return false;
	
	int bytesWritten = write(_pipe_in[1], _inputBuffer.c_str(), _inputBuffer.length());
	if (bytesWritten > 0)
	{
		_inputBuffer.erase(0, bytesWritten);
		if (_inputBuffer.empty())
		{
			close(_pipe_in[1]);
			_pipe_in[1] = -1;
			_state = CGI_READING;
		}
		return true;
	}

	else if (bytesWritten == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		_state = CGI_ERROR;
		return false;
	}
	return false;
}


bool	CgiHandler::readFromCgi()
{
	if (_pipe_out[0] == -1)
		return false;

	char buffer[4096];
	int bytesRead = read(_pipe_out[0], buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		_outputBuffer += buffer;
		return true;
	}

	else if (bytesRead == 0)
	{
		close(_pipe_out[0]);
		_pipe_out[0] = -1;

		int status;
		waitpid(_pid, &status, WNOHANG);
		_pid = -1;

		_state = CGI_DONE;
		return true;
	}

	else
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return true;
		_state = CGI_ERROR;
		return false;
	}
}

bool	CgiHandler::checkTimeout(long timeoutSeconds)
{
	if (_pid > 0 && (_state == CGI_WRITING || _state == CGI_READING))
	{
		struct timeval currenTime;
		gettimeofday(&currenTime, NULL);
		long elapsed = currenTime.tv_sec - _startTime.tv_sec;

		if (elapsed >= timeoutSeconds)
		{
			std::cerr << "CGI Timeout Exceeded!" << std::endl;
			killCgi();
			_state = CGI_ERROR;
			return true;
		}
	}
	return false;
}


void	CgiHandler::killCgi()
{
	if (_pid > 0)
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, WNOHANG);
		_pid = -1;
	}
}


//getters

int	CgiHandler::getWriteFd() const { return _pipe_in[1];}

int CgiHandler::getReadFd() const { return _pipe_out[0];}

CgiHandler::CgiState CgiHandler::getState() const { return _state; }

std::string CgiHandler::getOutput() const { return _outputBuffer; }

