#include "pch.h"
#include "Server.h"

int main()
{
	LogManager::Init();
	LOG_I("LogManager initialized.");

	Server& server = Server::GetInst();

	if (!server.Init())
	{
		LOG_E("Server initialization failed.");
		return EXIT_FAILURE;
	}

	server.Run();
	server.Shutdown();

	LogManager::Shutdown();
	return EXIT_SUCCESS;
}
