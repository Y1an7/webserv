#include "CgiHandler.hpp"

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