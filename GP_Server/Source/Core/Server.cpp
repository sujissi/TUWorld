#include "pch.h"
#include "Server.h"
#include "IOCP.h"
#include "SessionManager.h"
#include "GameWorldManager.h"

bool Server::Init()
{
	SetConsoleOutputCP(CP_UTF8);

	if (!ConfigManager::GetInst().Load())
	{
		LOG_E("ConfigManager");
		return false;
	}

#ifdef DB_MODE
	constexpr int32 dbThreadNum = 2;
	auto& cfg = ConfigManager::GetInst();
	if (!DBManager::GetInst().Connect(cfg.GetDBHost(), cfg.GetDBUser(), cfg.GetDBPassword(), cfg.GetDBSchema(), dbThreadNum))
	{
		LOG_E("DBManager");
		return false;
	}
	DBWorker::GetInst().Start(dbThreadNum);
#endif
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
	{
		LOG_E("WSAStartup");
		return false;
	}

	InitSocket(_listenSocket, WSA_FLAG_OVERLAPPED);
	if (_listenSocket == INVALID_SOCKET)
	{
		LOG_E("WSASocket");
		return false;
	}

	SOCKADDR_IN addr_s;
	addr_s.sin_family = AF_INET;
	addr_s.sin_port = htons(static_cast<u_short>(ConfigManager::GetInst().GetServerPort()));
	addr_s.sin_addr.s_addr = htonl(ADDR_ANY);

	if (bind(_listenSocket, reinterpret_cast<sockaddr*>(&addr_s), sizeof(addr_s)) == SOCKET_ERROR)
	{
		LOG_E("bind");
		return false;
	}

	if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		LOG_E("listen");
		return false;
	}

	if (!IOCP::GetInst().Init())
	{
		LOG_E("IOCP");
		return false;
	}
	IOCP::GetInst().RegisterSocket(_listenSocket);
	if (!SessionManager::GetInst().Init())
	{
		LOG_E("SessionManager");
		return false;
	}

	const std::string& dtPath = ConfigManager::GetInst().GetDataTablePath();
	bool res =
		ItemTable::GetInst().LoadFromCSV(dtPath + "ItemTable.csv") &&
		PlayerLevelTable::GetInst().LoadFromCSV(dtPath + "PlayerLevelTable.csv") &&
		PlayerSkillTable::GetInst().LoadFromCSV(dtPath + "PlayerSkillTable.csv") &&
		MonsterTable::GetInst().LoadFromCSV(dtPath + "MonsterTable.csv") &&
		SpawnTable::GetInst().LoadFromCSV(dtPath + "SpawnTable.csv") &&
		QuestTable::GetInst().LoadFromCSV(dtPath + "QuestTable.csv");

	if (!res)
	{
		LOG_E("LoadFromCSV");
		return false;
	}

	if (!Map::GetInst().Init())
	{
		LOG_E("MapZone");
		return false;
	}

	if (!GameWorldManager::GetInst().Init())
	{
		LOG_E("GameMgr");
		return false;
	}

	LOG_I("Successfully Init");
	return true;
}

void Server::Run()
{
	LOG_I("Run Server");
	DoAccept();

	static std::vector<std::thread> threads;
	int32 coreNum = std::thread::hardware_concurrency();
	for (int32 i = 0; i < coreNum; ++i)
	{
		threads.emplace_back(&Server::NetworkWorkerLoop, this);
		threads.emplace_back(&SessionManager::GameLogicWorkerLoop, &SessionManager::GetInst());
	}

	threads.emplace_back(&TimerQueue::TimerWorkerLoop);

	for (auto& thread : threads)
	{
		thread.join();
	}
}

void Server::Shutdown()
{
	LOG_I("Shutdown Server");
	_bRunning = false;

#ifdef DB_MODE
	DBWorker::GetInst().Shutdown();
#endif

	if (_listenSocket != INVALID_SOCKET) {
		closesocket(_listenSocket);
		_listenSocket = INVALID_SOCKET;
	}

	WSACleanup();
}

void Server::InitSocket(SOCKET& socket, DWORD dwFlags)
{
	socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, dwFlags);
}

void Server::NetworkWorkerLoop()
{
	DWORD recvByte;
	ULONG_PTR sessionId;
	LPWSAOVERLAPPED over;

	while (_bRunning)
	{
		BOOL ret = IOCP::GetInst().GetCompletion(recvByte, sessionId, over);
		ExpOver* expOver = reinterpret_cast<ExpOver*>(over);
		DWORD errorCode = (ret == FALSE) ? GetLastError() : NO_ERROR;

		if (!ret)
		{
			expOver->errorCode = errorCode;
			HandleCompletionError(expOver, static_cast<int32>(sessionId));
			continue;
		}

		switch (expOver->_compType)
		{
		case CompType::ACCEPT:
			SessionManager::GetInst().Connect(_acceptSocket);
			DoAccept();
			break;
		case CompType::RECV:
			SessionManager::GetInst().OnRecv(static_cast<int32>(sessionId), recvByte, expOver);
			SessionManager::GetInst().DoRecv(static_cast<int32>(sessionId));
			break;
		case CompType::SEND:
			delete over;
			break;
		}
	}
}

void Server::HandleCompletionError(ExpOver* over, int32 id)
{
	LPVOID msgBuf = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		over->errorCode,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&msgBuf,
		0,
		nullptr
	);

	std::string errMsg = msgBuf ? (char*)msgBuf : "Unknown error";
	if (msgBuf) LocalFree(msgBuf);

	switch (over->_compType)
	{
	case CompType::ACCEPT:
	{
		LOG_W("CompType : ACCEPT[{}] Code={}", id, over->errorCode);
		break;
	}
	case CompType::RECV:
	case CompType::SEND:
	{
		LOG_W("CompType : {}[{}] Code={}",
			(over->_compType == CompType::RECV ? "RECV" : "SEND"),
			id, over->errorCode);

		auto session = SessionManager::GetInst().GetSession(id);
		if (session && session->IsInGame())
		{
			EWorldChannel channel = session->GetWorldChannel();
			auto world = GameWorldManager::GetInst().GetWorld(channel);
			if (world)
				world->PlayerLeaveGame(id);
		}

		SessionManager::GetInst().Disconnect(id);

		if (over->_compType == CompType::SEND)
			delete over;

		break;
	}

	}

}

void Server::DoAccept()
{
	InitSocket(_acceptSocket, WSA_FLAG_OVERLAPPED);
	ZeroMemory(&_acceptOver._wsaover, sizeof(_acceptOver._wsaover));
	_acceptOver._compType = CompType::ACCEPT;
	bool ret = AcceptEx(_listenSocket, _acceptSocket, _acceptOver._buf, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &_acceptOver._wsaover);
	if (ret == FALSE && WSAGetLastError() != ERROR_IO_PENDING)
	{
		LOG_E("AcceptEx failed: {}", WSAGetLastError());
	}
}
