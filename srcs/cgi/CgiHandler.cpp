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
#include <climits>

CgiHandler::CgiHandler() : _envp(NULL), _argv(NULL), _pid(-1)
{
	_pipe_in[0] = -1;
	_pipe_in[1] = -1;
	_pipe_out[0] = -1;
	_pipe_out[1] = -1;
	_inputBytesSent = 0;
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

void    CgiHandler::_buildArgv(const CgiRequest& req)
{
    if (!req.executorPath.empty())
    {
        _argv = new char*[3]; 

        _argv[0] = new char[req.executorPath.length() + 1];
        size_t i = 0;
        while (i < req.executorPath.length()) {
            _argv[0][i] = req.executorPath[i];
            i++;
        }
        _argv[0][i] = '\0';

        _argv[1] = new char[req.scriptPath.length() + 1];
        size_t j = 0;
        while (j < req.scriptPath.length()) {
            _argv[1][j] = req.scriptPath[j];
            j++;
        }
        _argv[1][j] = '\0';

        _argv[2] = NULL;
    }
    else
    {
        _argv = new char*[2]; 

        _argv[0] = new char[req.scriptPath.length() + 1];
        size_t i = 0;
        while (i < req.scriptPath.length()) {
            _argv[0][i] = req.scriptPath[i];
            i++;
        }
        _argv[0][i] = '\0';

        _argv[1] = NULL;
    }
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
	envVars.push_back("PATH_INFO=" + req.uri);
	envVars.push_back("SCRIPT_NAME=" +req.uri);
	envVars.push_back("REQUEST_URI=" + req.uri);
	envVars.push_back("REDIRECT_STATUS=200");

	std::string host = "127.0.0.1";
	std::string port = "8080";
	if (req.headerInfo.find("host") != req.headerInfo.end())
	{
		std::string hostHeader = req.headerInfo.at("host");
		size_t colonPos = hostHeader.find(':');
		if (colonPos != std::string::npos)
		{
			host = hostHeader.substr(0, colonPos);
			port = hostHeader.substr(colonPos + 1);
		}
		else
			host = hostHeader;
	}
	envVars.push_back("SERVER_NAME=" + host);
	envVars.push_back("SERVER_PORT=" + port);

	if (req.headerInfo.find("content-length") != req.headerInfo.end())
		envVars.push_back("CONTENT_LENGTH=" + req.headerInfo.at("content-length"));
	else if (!req.httpBody.empty())
	{
		std::stringstream ss;
		ss << req.httpBody.length();
		envVars.push_back("CONTENT_LENGTH=" + ss.str());
	}

	if (req.headerInfo.find("content-type") != req.headerInfo.end())
		envVars.push_back("CONTENT_TYPE=" + req.headerInfo.at("content-type"));

	std::map<std::string, std::string>::const_iterator it = req.headerInfo.begin();
	
	while (it != req.headerInfo.end())
	{
		std::string key = it->first;
		if (key != "content-length" && key != "content-type")
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


void CgiHandler::reset()
{
	killCgi();
	if (_pipe_in[0] != -1)
	{
		close(_pipe_in[0]);
		_pipe_in[0] = -1;
	}
	if (_pipe_in[1] != -1)
	{
		close(_pipe_in[1]);
		_pipe_in[1] = -1;
	}
	if (_pipe_out[0] != -1)
	{
		close(_pipe_out[0]);
		_pipe_out[0] = -1;
	}
	if (_pipe_out[1] != -1)
	{
		close(_pipe_out[1]);
		_pipe_out[1] = -1;
	}
	_freeArray(_envp);
	_envp = NULL;
	_freeArray(_argv);
	_argv = NULL;
	_inputBuffer.clear();
	_inputBytesSent = 0;
	_outputBuffer.clear();
	_scriptDir.clear();
	_state = CGI_INIT;
}

bool CgiHandler::initCgi(const CgiRequest& req)
{
	CgiRequest abs = req;
	std::string dir = ".";
	std::string base = abs.scriptPath;
	std::string::size_type slash = abs.scriptPath.find_last_of('/');
	if (slash != std::string::npos)
	{
		dir = abs.scriptPath.substr(0, slash); //directory
		base = abs.scriptPath.substr(slash +1); //filename
	}
	char realDir[PATH_MAX];
	if (realpath(dir.c_str(), realDir) != NULL)
	{
		_scriptDir = realDir;
		abs.scriptPath = std::string(realDir) + "/" + base;
	}
	else
		_scriptDir = dir;

	if (!abs.executorPath.empty())
	{
		char realExec[PATH_MAX];
		if(realpath(abs.executorPath.c_str(), realExec) != NULL)
			abs.executorPath = realExec;
	}

	_buildEnvp(abs);
	_buildArgv(abs);

	_inputBuffer.swap(const_cast<std::string&>(req.httpBody));
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
	_lastProgress = _startTime;

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
		if (!_scriptDir.empty() && chdir(_scriptDir.c_str()) == -1)
			_exit(1);
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
		
	if (_inputBytesSent >= _inputBuffer.length())
	{
		close(_pipe_in[1]);
		_pipe_in[1] = -1;
		_state = CGI_READING;
		return true;
	}

	const char* dataPtr = _inputBuffer.c_str() + _inputBytesSent;
	size_t bytesLeft = _inputBuffer.length() - _inputBytesSent;
	if (bytesLeft > 65536)
		bytesLeft = 65536;
	ssize_t bytesWritten = write(_pipe_in[1], dataPtr, bytesLeft);

	if (bytesWritten > 0)
	{
		gettimeofday(&_lastProgress, NULL);
		_inputBytesSent += static_cast<size_t>(bytesWritten);
		if (_inputBytesSent >= _inputBuffer.length())
		{
			close(_pipe_in[1]);
			_pipe_in[1] = -1;
			_state = CGI_READING; 
		}
		return true;
	}
	return true;
}

bool	CgiHandler::readFromCgi()
{
	if (_pipe_out[0] == -1)
		return false;

	char 	buffer[4096];
	ssize_t bytesRead = read(_pipe_out[0], buffer, sizeof(buffer));

	if (bytesRead > 0)
	{
		gettimeofday(&_lastProgress, NULL);
		_outputBuffer.append(buffer, static_cast<size_t>(bytesRead));
		return true;
	}

	else if (bytesRead == 0)
	{
		close(_pipe_out[0]);
		_pipe_out[0] = -1;

		int status;
		if (waitpid(_pid, &status, WNOHANG) > 0)
			_pid = -1;

		_state = CGI_DONE;
		return true;
	}
	return true;
}

bool	CgiHandler::checkTimeout(long timeoutSeconds)
{
	if (_pid > 0 && (_state == CGI_WRITING || _state == CGI_READING))
	{
		struct timeval currenTime;
		gettimeofday(&currenTime, NULL);
		long elapsed = currenTime.tv_sec - _lastProgress.tv_sec;

		if (elapsed >= timeoutSeconds)
		{
			std::cerr << "CGI Timeout Exceeded!" << std::endl;
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
		waitpid(_pid, NULL, 0);
		_pid = -1;
	}
	if (_pipe_in[1] != -1)
	{
		close(_pipe_in[1]);
		_pipe_in[1] = -1;
	}
	if (_pipe_out[0] != -1)
	{
		close(_pipe_out[0]);
		_pipe_out[0] = -1;
	}
}


//getters

int	CgiHandler::getWriteFd() const { return _pipe_in[1];}

int CgiHandler::getReadFd() const { return _pipe_out[0];}

CgiHandler::CgiState CgiHandler::getState() const { return _state; }

const std::string& CgiHandler::getOutput() const { return _outputBuffer; }

void CgiHandler::clearOutput() { _outputBuffer.clear(); }

