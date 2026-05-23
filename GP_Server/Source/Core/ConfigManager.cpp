#include "pch.h"
#include "ConfigManager.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <fstream>
#include <sstream>

bool ConfigManager::Load(const std::string& configPath)
{
	std::ifstream file(configPath);
	if (!file.is_open())
	{
		LOG_E("ConfigManager: cannot open '{}'", configPath);
		return false;
	}

	std::stringstream ss;
	ss << file.rdbuf();

	rapidjson::Document doc;
	rapidjson::ParseResult result = doc.Parse(ss.str().c_str());
	if (!result)
	{
		LOG_E("ConfigManager: JSON parse error in '{}': {} (offset {})",
			configPath,
			rapidjson::GetParseError_En(result.Code()),
			result.Offset());
		return false;
	}

	if (doc.HasMember("paths") && doc["paths"].IsObject())
	{
		const auto& paths = doc["paths"];
		if (paths.HasMember("dataTable") && paths["dataTable"].IsString())
			_dataTablePath = paths["dataTable"].GetString();
		if (paths.HasMember("mapData") && paths["mapData"].IsString())
			_mapDataPath = paths["mapData"].GetString();
	}

	if (doc.HasMember("server") && doc["server"].IsObject())
	{
		const auto& server = doc["server"];
		if (server.HasMember("ip") && server["ip"].IsString())
			_serverIp = server["ip"].GetString();
		if (server.HasMember("port") && server["port"].IsInt())
			_serverPort = server["port"].GetInt();
	}

	if (doc.HasMember("database") && doc["database"].IsObject())
	{
		const auto& db = doc["database"];
		if (db.HasMember("host") && db["host"].IsString())
			_dbHost = db["host"].GetString();
		if (db.HasMember("user") && db["user"].IsString())
			_dbUser = db["user"].GetString();
		if (db.HasMember("password") && db["password"].IsString())
			_dbPassword = db["password"].GetString();
		if (db.HasMember("schema") && db["schema"].IsString())
			_dbSchema = db["schema"].GetString();
	}

	LOG_I("ConfigManager: loaded '{}'", configPath);
	return true;
}
