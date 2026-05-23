#pragma once
#include "NavMesh.h"

class Map
{
public:
	static Map& GetInst()
	{
		static Map inst;
		return inst;
	}

	bool Init(const std::string& mapDataPath = "");
	bool IsZoneAccessible(ZoneType zone, uint32 playerLevel) const;

	FVector GetRandomPos(ZoneType type) const;
	NavMesh& GetNavMesh(ZoneType type);
	
	FVector GetRandomEntryPos(ZoneType oldZone, ZoneType targetZone) const;
	FVector GetRandomEntryPos(EntryType entryType) const;
	NavMesh& GetEntryNavMesh(EntryType entryType);

	FVector GetStartPos(ZoneType startZone);
private:
	std::unordered_map<ZoneType, NavMesh> _navMeshes;
	std::unordered_map<EntryType, NavMesh> _entryMeshes;

};

