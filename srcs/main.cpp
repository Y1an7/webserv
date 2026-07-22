#include "incs/config/ConfigParser.hpp"
#include "incs/network/Server.hpp"
#include "incs/network/ServerSocket.hpp"
#include <iostream>
#include <exception>
#include <csignal>
#include <cstdlib>
#include <vector>


Server* g_server = NULL;


void handle_signal(int sig)
{
	(void)sig;
	std::cout << "\n[SIGINT/SIGQUIT] Shutting down webserv gracefully..." << std::endl;

	if (g_server)
	{
		delete g_server;
		g_server = NULL;
	}
	exit(0);
}


int main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	//1. validate arg
	if (argc > 2)
	{
		std::cerr << "Usage: ./webserv [config_file]" << std::endl;
		return 1;
	}

	std::string ConfigFile = (argc == 2) ? argv[1] : "default.conf";

	//2.register signal handlers
	signal(SIGINT, handle_signal);
	signal(SIGQUIT, handle_signal);

	try
	{
		// 3. Parse the config
		std::cout << "Parsing configuration file: " << std::endl;
		ConfigParser parser;
		parser.tokenize(ConfigFile);
		parser.parse();

		const std::vector<ServerConfig>& serverConfigs = parser.getServers();
		if (serverConfigs.empty())
		{
			std::cerr << "Error: No valid server blocks found in configuration file." << std::endl;
			return 1;
		}
		// 4.initialize the main server structure
		g_server = new Server();

		for (size_t i = 0; i < serverConfigs.size(); ++i)
		{
			ServerSocket* newSocket = new ServerSocket(serverConfigs[i]);
			try
			{
				newSocket->init();
				g_server->addServerSocket(newSocket);
				std::cout << "✅ Listening on Host: " << serverConfigs[i].getHost()
					<< "Port: " << serverConfigs[i].getPort() << std::endl;
			}
			catch (const std::exception& e)
			{
				delete newSocket;
				throw ;
			}
		}

		//5.initialize epoll and run the main event loop
		g_server->initEpoll();
		g_server->run();
	}

	catch (const std::exception& e)
	{
		std::cerr << "\n❌ Fatal Exception: " << e.what() << std::endl;
		if (g_server)
		{
			delete g_server;
			g_server = NULL;
		}
		return 1;
	}
	return 0;
}