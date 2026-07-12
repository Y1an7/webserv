#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

# include <iostream>
# include <string>
# include <map>
# include <vector>
# include <sys/types.h>
# include <sys/time.h>

struct	CgiRequest
{
	std::string							method;
	std::string							scriptPath;
	std::string							queryString;
	std::string							httpBody;
	std::map<std::string, std::string>	headerInfo;
};

class	CgiHandler
{
public:
	enum CgiState {
		CGI_INIT,
		CGI_WRITING,
		CGI_READING,
		CGI_DONE,
		CGI_ERROR
	};

private:
	char**			_envp; //hold the dynamically allocated env variables
	char**			_argv; //hold the argument for execve;
	pid_t			_pid; //to store the process id of the child clone so the parent can wait for it later

	int				_pipe_in[2]; //reading
	int				_pipe_out[2]; //writing

	CgiState		_state;
	std::string		_inputBuffer;
	std::string		_OutputBUffer;

	struct timeval	_startTime;

	void			_buildEnvp(const CgiRequest& req);
	void			_buildArgv(const CgiRequest& req);
	void			_freeArray(char** array);
	void			_setNonBlocking(int fd);

public:
	CgiHandler();
	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(const CgiHandler& other);
	~CgiHandler();

	bool			initCgi(const CgiRequest& req);

	bool			writeToCgi();
	bool			readFromCgi();

	int				getWriteFd() const;
	int				getReadFd() const;
	CgiState		getState() const;
	std::string		getOutput() const;

	bool			checkTimeout(long timeoutSeconds);
	void			killCgi();

	std::string	execute(const CgiRequest& req); //public interface aka entry point, std::string is the HTML result
};


#endif