#pragma once
#include <string>

class ConfigManager
{
public:
	static ConfigManager& GetInst()
	{
		static ConfigManager inst;
		return inst;
	}

	bool Load(const std::string& configPath = "config.json");

	const std::string& GetDataTablePath() const { return _dataTablePath; }
	const std::string& GetMapDataPath() const { return _mapDataPath; }
	const std::string& GetServerIP() const { return _serverIp; }
	int GetServerPort() const { return _serverPort; }
	const std::string& GetDBHost() const { return _dbHost; }
	const std::string& GetDBUser() const { return _dbUser; }
	const std::string& GetDBPassword() const { return _dbPassword; }
	const std::string& GetDBSchema() const { return _dbSchema; }

private:
	ConfigManager() = default;

	std::string _dataTablePath = "./DataTable/";
	std::string _mapDataPath = "./MapJsonData/";
	std::string _serverIp = "0.0.0.0";
	int _serverPort = 4000;
	std::string _dbHost = "localhost";
	std::string _dbUser;
	std::string _dbPassword;
	std::string _dbSchema;
};
