#pragma once
#include "Monster.h"
#include "PlayerSession.h"
#include "SessionManager.h"
#include "Item.h"
#include "WorldItem.h"

struct GridPos {
	int32 X, Y;

	bool operator==(const GridPos& other) const {
		return X == other.X && Y == other.Y;
	}
};

namespace std {
	template <>
	struct hash<GridPos> {
		size_t operator()(const GridPos& pos) const {
			return std::hash<int32>()(pos.X) ^ (std::hash<int32>()(pos.Y) << 1);
		}
	};
}

class GameWorld
{
public:

	bool Init(EWorldChannel channelId);

	// Player
	void PlayerEnterGame(std::shared_ptr<Player> player, bool bChange = false);
	void PlayerLeaveGame(int32 playerId);
	void PlayerSetLocation(int32 playerId, float yaw, FVector pos);
	void PlayerAddState(int32 playerId, ECharacterStateType newState);
	void PlayerRemoveState(int32 playerId, ECharacterStateType oldState);
	void PlayerMove(int32 playerId, FVector& pos, uint32 state, uint64& time);
	void PlayerAttack(int32 playerId);
	void PlayerUseSkill(int32 playerId, ESkillGroup groupId);
	void PlayerEndSkill(int32 playerId, ESkillGroup groupId);
	
	void PlayerDead(int32 playerID);

	// Monster
	void CreateMonster();
	void TestUpdateAll();
	void OnMonsterDead(int32 monsterId);
	void UpdateMonsterState(int32 id, ECharacterStateType newState);
	void MonsterRespawn(int32 monsterId);
	//Tino
	void HandleEarthQuakeImpact(const FVector& rockPos);

	// Inventory
	void PickUpWorldItem(int32 playerId, uint32 itemId);
	void UseInventoryItem(int32 playerId, uint32 itemId);
	void EquipInventoryItem(int32 playerId, uint32 itemId);
	void UnequipInventoryItem(int32 playerId, uint32 itemId);

	// World Item
	bool RemoveWorldItem(uint32 itemId, ZoneType zone);
	std::shared_ptr<WorldItem> FindWorldItemById(uint32 itemId, ZoneType zone);
	void SpawnGoldItem(FVector position, ZoneType zone);

	void SpawnWorldItem(FVector position, uint32 monlv, Type::EPlayer playertype, ZoneType zone);
	void SpawnWorldItem(WorldItem newItem, ZoneType zone);
	void DespawnWorldItem(uint32 itemId, ZoneType zone);

	void ClearItems(int32 playerId, ZoneType oldZone);
	void AddItems(int32 playerId, ZoneType oldZone);
	
	// Etc
	bool TransferToZone(int32 playerId, ZoneType newZone);
	void RespawnPlayer(int32 playerId, ZoneType newZone);
	void UpdateViewList(int32 playerId);
	void ClearViewList(int32 playerId);
	void InitViewList(int32 playerId, ZoneType zone);
	void AddAllToViewList(std::shared_ptr<Player> player, ZoneType zone);
	bool IsCollisionDetected(ZoneType zone, const FVector& pos, float dist);

	//Quest
	void RequestQuest(int32 playerId, QuestType quest);
	void CompleteQuest(int32 playerId, QuestType quest, bool force);
	void RejectQuest(int32 playerId, QuestType quest);
	void QuestSpawn(int32 playerId, QuestType quest);
	void BuyItem(int32 playerId, uint8 itemType, uint16 quantity);
	void SellItem(int32 playerId, uint32 itemId);

	void BroadcastToZone(ZoneType zone, Packet* packet);

	void EnterGrid(int32 id, const FVector& pos);
	void LeaveGrid(int32 id, const FVector& pos);
	void MoveGrid(int32 id, const FVector& oldPos, const FVector& newPos);
	void UpdateMoveGridInTUK(int32 id, const FVector& oldPos, const FVector& pos);
	std::vector<int32> QueryNearbyCharacters(const FVector& pos);
	GridPos GetGridPos(const FVector& pos);

	std::shared_ptr<Player> GetPlayerByID(int32 id);
	std::shared_ptr<Monster> GetMonsterByID(int32 id);
	std::shared_ptr<Character> GetCharacterByID(int32 id);
	int32 GetMonsterCnt(ZoneType zone) { return _monsterCnt[zone]; }
	bool IsMonster(int32 id);
	int32 GenerateMonsterId() { return _nextMonsterId++; }

	int32 GetPlayerCount() {
		std::lock_guard<std::mutex> lock(_mtPlayers);
		return static_cast<int32>(_players.size());
	}
	EWorldChannel GetChannelId() const { return _channelId; }
private:
	EWorldChannel _channelId;
	std::unordered_map<int32, std::shared_ptr<Player>> _players;
	std::unordered_map<QuestType, int32> _questMons;
	std::unordered_map<ZoneType, std::unordered_map<int32, std::shared_ptr<Player>>> _playersByZone;
	std::unordered_map<ZoneType, std::unordered_map<int32, std::shared_ptr<Monster>>> _monstersByZone;
	std::unordered_map<int32, std::shared_ptr<Monster>> _allMonsters;
	std::unordered_map<ZoneType, std::unordered_map<uint32, std::shared_ptr<WorldItem>>> _worldItemsByZone;

	std::mutex _mtPlayers;
	std::mutex _mtPlayerZMap;
	std::mutex _mtMonZMap;
	std::mutex _mtItemZMap;
	int32 _nextMonsterId = MAX_PLAYER;
	std::unordered_map<ZoneType, int32> _monsterCnt;

private:
	std::unordered_map<QuestType, std::vector<std::shared_ptr<Monster>>> _activeQuestMonsters;
	std::mutex _mtQuestMonsters;

private:
	std::unordered_map<GridPos, std::unordered_set<int32>> _gridMap;
	std::mutex _gridMutex;
};

