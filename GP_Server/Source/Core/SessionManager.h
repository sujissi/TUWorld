#pragma once
#include "PlayerSession.h"
#include "IOCP.h"
#include "GameJobScheduler.h"
class SessionManager
{
public:
	static SessionManager& GetInst()
	{
		static SessionManager inst;
		return inst;
	}
	bool Init();
	void Connect(SOCKET& socket);
	void Disconnect(int32 sessionId);
	void GameLogicWorkerLoop();
	void Schedule(int32 sessionId, std::function<void()> job);

	void DoRecv(int32 sessionId);
	void OnRecv(int32 sessionId, int32 recvByte, ExpOver* expOver);
	void HandleLogin(int32 sessionId, const DBLoginResult& dbRes = DBLoginResult());
	void SendPacket(int32 sessionId, const Packet* packet);
	void BroadcastToAll(Packet* packet);
	void BroadcastToFriends(int32 sessionId, Packet* packet);
	void BroadcastToViewList(Packet* packet, const std::unordered_set<int32>& viewList);
	std::array<std::unique_ptr<PlayerSession>, MAX_CLIENT>& GetSessions() { return _sessions; }
	PlayerSession* GetSession(int32 sessionId);
	int32 GetOnlineSessionIdByDBId(uint32 dbid);

	uint32 FindDBIdByName(const std::string& name);
	std::string FindNameByDBId(uint32 dbid);
	int32 FindSessionIdByName(const std::string& name);
	void BroadcastWorldState(FWorldState worldState);

private:
	int32 GenerateId();
private:
	std::array<std::unique_ptr<PlayerSession>, MAX_CLIENT> _sessions;
	IOCP& _iocp = IOCP::GetInst();
	GameJobScheduler& _jobManager = GameJobScheduler::GetInst();
	std::queue<int32> _freeIds;
	std::mutex _sMutex;
	std::mutex _idMutex;
private:
	std::unordered_map<uint32, int32> _dbidToSessionIdMap;
	std::unordered_map<uint32, std::string> _dbidToNameMap;
	std::unordered_map<std::string, uint32> _nameToDbidMap;
	std::mutex _mapMutex;

};

