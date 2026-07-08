#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

# include <iostream>
# include <string>
# include <map>
# include <unistd.h>
# include <vector>

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
private:
	char**	_envp; //hold the dynamically allocated env variables
	char**	_argv; //hold the argument for execve;
	int		_pipe_in[2]; //reading
	int		_pipe_out[2]; //writing
	pid_t	_pid; //to store the process id of the child clone so the parent can wait for it later

	void	_buildEnvp(const CgiRequest& req);
	void	_buildArgv(const CgiRequest& req);
	void	_freeArray(char** array);

public:
	CgiHandler();
	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(const CgiHandler& other);
	~CgiHandler();

	std::string	execute(const CgiRequest& req); //public interface aka entry point, std::string is the HTML result
};


#endif