#include "pch.h"
#include "DummyClinetManager.h"

using namespace std::chrono;


bool DummyClientManager::Init()
{
	_hIocp.Init();
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOG_E("WSAStartup");
		return false;
	}
	for (uint32 i = 0;i < CLIENT_NUM;++i)
		_clients[i].Init(i);
	if (!Map::GetInst().Init())
	{
		LOG_E("MapZone");
		return false;
	}
	return true;
}

void DummyClientManager::Run()
{
	std::thread worker, timer;
	try {
		static std::vector<std::thread> threads;

		threads.emplace_back([this]() { WorkerThread(); });
		threads.emplace_back([this]() { TestThread(); });
		threads.emplace_back(TimerQueue::TimerWorkerLoop);
		for (auto& thread : threads)
		{
			thread.join();
		}
	}
	catch (const std::exception& e) {
		LOG_E("Thread error: {}", e.what());
		throw;
	}

}

void DummyClientManager::Shutdown()
{
	_running = false;
	for (auto& client : _clients)
		client.Disconnect();
	TimerQueue::_cv.notify_all();
}

void DummyClientManager::SendMovePacket(int i)
{
	if (_clients[i].IsConnected())
	{
		_clients[i].SendMovePacket();
	}
	else
	{
		LOG_E("client[{}] is not connected", i);
	}
}
void DummyClientManager::WorkerThread()
{
	LOG_I("Run Stress Test WorkerThread");
	DWORD rbyte;
	ULONG_PTR id;
	LPWSAOVERLAPPED over;
	static auto lastCheck = std::chrono::steady_clock::now();

	try {
		while (_running)
		{
			BOOL ret = _hIocp.GetCompletion(rbyte, id, over);
			ExpOver* expOver = reinterpret_cast<ExpOver*>(over);

			if (!ret || rbyte == 0)
			{
				expOver->errorCode = GetLastError();
				HandleCompletionError(expOver, static_cast<int32>(id));
				continue;
			}

			switch (expOver->_compType)
			{
			case CompType::RECV:
				HandleRecv(static_cast<int32>(id), rbyte, over);
				break;
			case CompType::SEND:
				delete over;
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		LOG_E("Exception in WorkerThread: {}", e.what());
	}
	catch (...)
	{
		LOG_E("Unknown exception in WorkerThread!");
	}
}

void DummyClientManager::TestThread()
{
	while (true)
	{
		AdjustClientCount();
		for (int i = 0;i < CLIENT_NUM;i++)
		{
			if (!_clients[i].IsConnected()) continue;
			if (_clients[i].IsLogin())
				_clients[i].SendMovePacket();
		}
	}
}

void DummyClientManager::HandleRecv(int32 id, DWORD rbyte, LPWSAOVERLAPPED over)
{
	_clients[id].HandleRecvBuffer(rbyte, reinterpret_cast<ExpOver*>(over));
	_clients[id].DoRecv();
}

void DummyClientManager::AdjustClientCount()
{
	static int delay_multiplier = 1;
	static int max_limit = MAXINT;
	static bool increasing = true;
	if (_active_clients >= CLIENT_NUM) return;
	if (_num_connections >= CLIENT_NUM) return;
	auto duration = high_resolution_clock::now() - last_connect_time;
	if (ACCEPT_DELY * delay_multiplier > duration_cast<milliseconds>(duration).count()) return;

	int t_delay = delayTime;
	if (DELAY_LIMIT2 < t_delay) {
		if (true == increasing) {
			max_limit = _active_clients;
			increasing = false;
		}
		if (100 > _active_clients) return;
		if (ACCEPT_DELY * 10 > duration_cast<milliseconds>(duration).count()) return;
		last_connect_time = high_resolution_clock::now();
		Disconnect(_client_to_close);
		_client_to_close++;
		return;
	}
	else
		if (DELAY_LIMIT < t_delay) {
			delay_multiplier = 10;
			return;
		}
	if (max_limit - (max_limit / 20) < _active_clients) return;
	increasing = true;
	last_connect_time = high_resolution_clock::now();
	Connect(_num_connections);
}

void DummyClientManager::HandleCompletionError(ExpOver* ex_over, int32 id)
{
	LPVOID msgBuf = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		ex_over->errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&msgBuf,
		0,
		nullptr
	);

	std::string errMsg = msgBuf ? (char*)msgBuf : "Unknown error";
	if (msgBuf) LocalFree(msgBuf);

	if (!_clients[id].IsConnected())
	{
		if (ex_over->_compType == CompType::SEND)
			delete ex_over;
		return;
	}
	std::string cmptype;
	switch (ex_over->_compType)
	{
	case CompType::ACCEPT: cmptype = "ACCEPT"; break;
	case CompType::RECV: cmptype = "RECV"; break;
	case CompType::SEND: cmptype = "SEND"; break;
	}

	LOG_W("CompType : {}[{}] Code={} Msg={}", cmptype, id, ex_over->errorCode, errMsg);

	Disconnect(id);

	if (ex_over->_compType == CompType::SEND)
		delete ex_over;
}
