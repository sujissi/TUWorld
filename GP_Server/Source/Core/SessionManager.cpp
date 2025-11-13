#include "pch.h"
#include "SessionManager.h"
#include "PacketManager.h"

bool SessionManager::Init()
{
	for (int i = 1; i < MAX_CLIENT; ++i)
		_freeIds.push(i);
	return true;
}

void SessionManager::Connect(SOCKET& socket)
{
	std::lock_guard<std::mutex> lock(_sMutex);
	int32 _id = GenerateId();
	if (_id != -1) {
		_sessions[_id] = std::make_unique<PlayerSession>();
		_sessions[_id]->Connect(socket, _id);
		_iocp.RegisterSocket(socket, _id);
		_sessions[_id]->DoRecv();
	}
}

void SessionManager::Disconnect(int32 sessionId)
{
	{
		std::lock_guard<std::mutex> lock(_sMutex);

		if (sessionId < 0 || sessionId >= MAX_CLIENT || !_sessions[sessionId])
		{
			LOG_W("Invalid session id: {}", sessionId);
			return;
		}
		auto session = _sessions[sessionId].get();
		auto name = session->GetNickName();
		auto dbId = session->GetDBID();
		{
			std::lock_guard<std::mutex> lock(_mapMutex);
			_dbidToSessionIdMap.erase(dbId);
			_dbidToNameMap.erase(dbId);
			_nameToDbidMap.erase(name);
		}
		_sessions[sessionId]->Disconnect();
		_sessions[sessionId].reset();
	}

	{
		std::lock_guard<std::mutex> idLock(_idMutex);
		_freeIds.push(sessionId);
	}
}


void SessionManager::Schedule(int32 sessionId, std::function<void()> job)
{
	auto session = GetSession(sessionId);
	if (!session)
	{
		LOG_W("Invalid");
		return;
	}
	session->PushJob(job);
	GameJobScheduler::GetInst().Schedule(sessionId);
}

void SessionManager::GameLogicWorkerLoop()
{
	while (true) {
		int32 sessionId = GameJobScheduler::GetInst().Pop();
		auto session = GetSession(sessionId);
		if (!session)
		{
			LOG_W("Invalid");
			return;
		}
		session->RunGameJobs();
	}
}

void SessionManager::DoRecv(int32 sessionId)
{
	auto session = GetSession(sessionId);
	if (!session)
	{
		LOG_W("Invalid");
		return;
	}
	session->DoRecv();
}

void SessionManager::OnRecv(int32 sessionId, int32 recvByte, ExpOver* expOver)
{
	auto session = GetSession(sessionId);
	if (!session)
	{
		LOG_W("Invalid");
		return;
	}
	session->OnRecv(recvByte, expOver);
}

void SessionManager::HandleLogin(int32 sessionId, const DBLoginResult& dbRes)
{
	auto session = GetSession(sessionId);
	if (!session)
	{
		LOG_W("Invalid");
		return;
	}
	session->Login(dbRes);
	auto& playerInfo = session->GetPlayerInfo();
#ifdef DB_MODE
	auto name = session->GetNickName();
	auto dbId = session->GetDBID();
	{
		std::lock_guard<std::mutex> lock(_mapMutex);
		if (_dbidToSessionIdMap.contains(dbId))
		{
			LOG_W("Duplicate login detected: DBID {}", dbId);
			//Todo: 
		}
		_dbidToSessionIdMap[dbId] = sessionId;
		_dbidToNameMap[dbId] = name;
		_nameToDbidMap[name] = dbId;
	}
#endif
}

void SessionManager::SendPacket(int32 sessionId, const Packet* packet)
{
	auto session = GetSession(sessionId);
	if (!session)
	{
		LOG_W("Invalid");
		return;
	}
	session->DoSend(packet);
}

void SessionManager::BroadcastToAll(Packet* packet)
{
	for (auto& session : _sessions)
		if (session && session->IsLogin())
			session->DoSend(packet);
}

void SessionManager::BroadcastToFriends(int32 sessionId, Packet* packet)
{
	auto session = GetSession(sessionId);
	if (!session)
	{
		LOG_W("Invalid");
		return;
	}
	const auto& friends = session->GetFriends();
	for (const auto& f : friends)
	{
		int32 fsessId = GetOnlineSessionIdByDBId(f.DBId);
		if (fsessId != -1)
		{
			SendPacket(fsessId, packet);
		}
	}
}

void SessionManager::BroadcastToViewList(Packet* packet, const std::unordered_set<int32>& viewList)
{
	for (int32 sessionId : viewList)
	{
		if (sessionId < 0 || sessionId >= MAX_CLIENT)
			continue;

		auto session = GetSession(sessionId);
		if (!session)
			continue;

		session->DoSend(packet);
	}
}

PlayerSession* SessionManager::GetSession(int32 sessionId)
{
	std::lock_guard<std::mutex> lock(_sMutex);
	if (sessionId < 0 || sessionId >= MAX_CLIENT || !_sessions[sessionId])
		return nullptr;
	return _sessions[sessionId].get();
}

int SessionManager::GenerateId()
{
	std::lock_guard<std::mutex> lock(_idMutex);
	if (_freeIds.empty())
		return -1;

	int id = _freeIds.front();
	_freeIds.pop();
	return id;
}

int32 SessionManager::GetOnlineSessionIdByDBId(uint32 dbid)
{
	std::lock_guard<std::mutex> lock(_mapMutex);
	auto it = _dbidToSessionIdMap.find(dbid);

	int32 sessId = -1;
	if (it != _dbidToSessionIdMap.end())
		sessId = it->second;
	auto sess = GetSession(sessId);
	if (sess && sess->IsLogin())
		return sessId;
	return -1;
}

uint32 SessionManager::FindDBIdByName(const std::string& name)
{
	std::lock_guard<std::mutex> lock(_mapMutex);
	auto it = _nameToDbidMap.find(name);
	if (it != _nameToDbidMap.end())
		return it->second;
	return 0;
}

std::string SessionManager::FindNameByDBId(uint32 dbid)
{
	std::lock_guard<std::mutex> lock(_mapMutex);
	auto it = _dbidToNameMap.find(dbid);
	if (it != _dbidToNameMap.end())
		return it->second;
	return "";
}

int32 SessionManager::FindSessionIdByName(const std::string& name)
{
	std::lock_guard<std::mutex> lock(_mapMutex);

	auto dbidIt = _nameToDbidMap.find(name);
	if (dbidIt == _nameToDbidMap.end())
		return -1;

	uint32 dbid = dbidIt->second;

	auto sessionIt = _dbidToSessionIdMap.find(dbid);
	if (sessionIt == _dbidToSessionIdMap.end())
		return -1;

	return sessionIt->second;
}

void SessionManager::BroadcastWorldState(FWorldState worldState)
{
	auto pkt = ChangedWorldStatePacket(worldState);
	for (auto& session : SessionManager::GetInst().GetSessions())
	{
		if (session && session->IsLogin())
		{
			session->DoSend(&pkt);
		}
	}
}