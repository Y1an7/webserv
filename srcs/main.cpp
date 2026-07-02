#include "incs/config/ConfigParser.hpp"
#include "incs/network/ServerSocket.hpp" // Adjust path if needed
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv [config_file]" << std::endl;
		return 1;
	}

	try
	{
		// 1. Parse the data
		ConfigParser parser;
		parser.tokenize(argv[1]);
		parser.parse();

		std::cout << "Parser complete. Found " << parser.getServers().size() << " servers." << std::endl;

		// 2. Extract the first server block
		const ServerConfig& firstConfig = parser.getServers()[0];

		// 3. Connect to socket
		ServerSocket testSocket(firstConfig);
		testSocket.init();
		
		std::cout << "Success! Listening on port: " << firstConfig.getPort() << std::endl;
		std::cout << "Open another terminal and type: curl -v http://localhost:" << firstConfig.getPort() << std::endl;

		// 4. Dummy accept loop for testing
		// Note: Because ServerSocket uses O_NONBLOCK, this will spin fast
		while (true)
		{
			int client_fd = testSocket.acceptConnect();
			if (client_fd > 0)
			{
				std::cout << "✅ BOOM! Connection accepted on FD: " << client_fd << std::endl;
				close(client_fd); // Close it immediately for this test
			}
			usleep(10000); // Sleep for 10ms to prevent 100% CPU usage
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}