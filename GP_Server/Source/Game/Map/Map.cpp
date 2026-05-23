#include "pch.h"
#include "Map.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

bool Map::Init()
{
	const std::vector<std::pair<ZoneType, std::string>> ZoneMeshFiles = {
	{ ZoneType::TUK,        "NavMeshData_TUK.json" },
	{ ZoneType::TIP,        "NavMeshData_TIP.json" },
	{ ZoneType::E,          "NavMeshData_E.json" },
	{ ZoneType::GYM,        "NavMeshData_GYM.json" },
	{ ZoneType::INDUSTY,    "NavMeshData_Industry.json" },
	{ ZoneType::BUNKER,     "NavMeshData_Bunker.json" }
	};
	const std::vector<std::pair<EntryType, std::string>> EntryMeshFiles = {
	{ EntryType::TIP_IN,       "NavMeshData_TIP_In.json" },
	{ EntryType::TIP_OUT,      "NavMeshData_TIP_Out.json" },
	{ EntryType::E_IN,         "NavMeshData_E_In.json" },
	{ EntryType::E_OUT,        "NavMeshData_E_Out.json" },
	{ EntryType::GYM_IN,       "NavMeshData_Gym_In.json" },
	{ EntryType::GYM_OUT,      "NavMeshData_Gym_Out.json" },
	{ EntryType::INDUSTY_IN,   "NavMeshData_Industry_In.json" },
	{ EntryType::INDUSTY_OUT,  "NavMeshData_Industry_Out.json" }
	};

	const std::string& mapPath = ConfigManager::GetInst().GetMapDataPath();
	for (const auto& [zone, file] : ZoneMeshFiles)
	{
		auto meshOpt = NavMesh::LoadFromJson(mapPath + file);
		if (!meshOpt)
		{
			LOG_E("Failed to load zone mesh [{}]", static_cast<int>(zone));
			return false;
		}
		_navMeshes[zone] = std::move(*meshOpt);
	}

	for (const auto& [entry, file] : EntryMeshFiles)
	{
		auto meshOpt = NavMesh::LoadFromJson(mapPath + file);
		if (!meshOpt)
		{
			LOG_E("Failed to load entry mesh [{}]", static_cast<int>(entry));
			return false;
		}
		_entryMeshes[entry] = std::move(*meshOpt);
	}

	return true;
}

bool Map::IsZoneAccessible(ZoneType zone, uint32 playerLevel) const
{
	switch (zone)
	{
	case ZoneType::TIP:       return playerLevel > 0;
	case ZoneType::TUK:       return playerLevel > 0;
	case ZoneType::E:         return playerLevel > 0;
	case ZoneType::INDUSTY:   return playerLevel > 0;
	case ZoneType::GYM:       return playerLevel > 0;
	default:                  return false;
	}
}

FVector Map::GetRandomPos(ZoneType type) const
{
	auto it = _navMeshes.find(type);
	if (it != _navMeshes.end())
		return it->second.GetRandomPosition();

	return FVector::ZeroVector;
}

NavMesh& Map::GetNavMesh(ZoneType type)
{
	auto it = _navMeshes.find(type);
	if (it == _navMeshes.end())
		LOG_W("Invalid");

	return it->second;
}

FVector Map::GetRandomEntryPos(ZoneType oldZone, ZoneType targetZone) const
{
	EntryType entryType;

	switch (targetZone)
	{
	case ZoneType::TIP:
		entryType = EntryType::TIP_IN;
		break;
	case ZoneType::E:
		entryType = EntryType::E_IN;
		break;
	case ZoneType::GYM:
		entryType = EntryType::GYM_IN;
		break;
	case ZoneType::INDUSTY:
		entryType = EntryType::INDUSTY_IN;
		break;
	case ZoneType::TUK:
	{
		if (oldZone == ZoneType::TIP)
			entryType = EntryType::TIP_OUT;
		else if (oldZone == ZoneType::E)
			entryType = EntryType::E_OUT;
		else if (oldZone == ZoneType::GYM)
			entryType = EntryType::GYM_OUT;
		else if (oldZone == ZoneType::INDUSTY)
			entryType = EntryType::INDUSTY_OUT;
		else
			return FVector::ZeroVector;
		break;
	}
	default:
		return FVector::ZeroVector;
	}

	return GetRandomEntryPos(entryType);
}

FVector Map::GetRandomEntryPos(EntryType entryType) const
{
	auto it = _entryMeshes.find(entryType);
	if (it != _entryMeshes.end())
		return it->second.GetRandomPosition();

	return FVector::ZeroVector;
}

NavMesh& Map::GetEntryNavMesh(EntryType entryType)
{
	return _entryMeshes.at(entryType);
}

FVector Map::GetStartPos(ZoneType startZone)
{
	switch (startZone)
	{
	case ZoneType::TIP:
	{
		FVector newPos;
		float radius = playerCollision;
		newPos = Map::GetInst().GetRandomPos(startZone);
		return newPos;
	}
	case ZoneType::TUK:
	{
		FVector newPos;
		float radius = playerCollision;
		newPos = Map::GetInst().GetRandomPos(startZone);
		return newPos;
	}
	}

}