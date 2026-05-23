#include "pch.h"
#include "DummyClinetManager.h"

using namespace std::chrono;


bool DummyClientManager::Init()
{
	_hIocp.Init();
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "WSAStartup failed\n";
		return false;
	}
	for (uint32 i = 0;i < CLIENT_NUM;++i)
		_clients[i].Init(i);
	if (!Map::GetInst().Init(MapDataPath))
	{
		std::cerr << "Map init failed\n";
		return false;
	}
	return true;
}

void DummyClientManager::Run()
{
	try {
		_threads.emplace_back([this]() { WorkerThread(); });
		_threads.emplace_back([this]() { TestThread(); });
		_threads.emplace_back([this]() { StatsThread(); });
		_threads.emplace_back(TimerQueue::TimerWorkerLoop);
		for (auto& t : _threads)
			if (t.joinable()) t.join();
	}
	catch (const std::exception& e) {
		std::cerr << "Thread error: " << e.what() << '\n';
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
		_clients[i].SendMovePacket();
}
void DummyClientManager::WorkerThread()
{
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
		std::cerr << "Exception in WorkerThread: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "Unknown exception in WorkerThread!\n";
	}
}

void DummyClientManager::TestThread()
{
	while (_running)
	{
		AdjustClientCount();
		for (int i = 0; i < CLIENT_NUM; i++)
		{
			if (!_clients[i].IsConnected()) continue;
			if (_clients[i].IsLogin())
				_clients[i].SendMovePacket();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
		// RTT 심각 (>300ms): 클라이언트 감소
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
	else if (DELAY_LIMIT < t_delay) {
		// RTT 경고 (100~300ms): 접속 속도 낮춤
		delay_multiplier = 10;
		return;
	}

	// RTT 양호 (<100ms): 접속 속도 정상화 후 클라이언트 추가
	delay_multiplier = 1;
	if (max_limit - (max_limit / 20) < _active_clients) return;
	increasing = true;
	last_connect_time = high_resolution_clock::now();
	Connect(_num_connections);
}

void DummyClientManager::StatsThread()
{
	auto startTime = steady_clock::now();

	std::cout << std::format("{:>5}  {:>6}  {:>6}  {:>8}  {:>9}  {:>7}  {:>7}  {:>7}\n",
		"sec", "conn", "active", "move/s", "avg(ms)", "min(ms)", "max(ms)", "delay");
	std::cout << std::string(68, '-') << '\n';

	while (_running)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		auto elapsedSec = duration_cast<seconds>(steady_clock::now() - startTime).count();

		long long sent    = g_stats.moveSent.exchange(0);
		long long rttSum  = g_stats.rttSum.exchange(0);
		int       rttCnt  = g_stats.rttCount.exchange(0);
		long long rttMin  = g_stats.rttMin.exchange(-1);
		long long rttMax  = g_stats.rttMax.exchange(-1);

		double avgRtt = (rttCnt > 0) ? static_cast<double>(rttSum) / rttCnt : 0.0;
		std::string minStr = (rttMin >= 0) ? std::format("{}ms", rttMin) : "-";
		std::string maxStr = (rttMax >= 0) ? std::format("{}ms", rttMax) : "-";

		std::cout << std::format("{:>4d}s  {:>6d}  {:>6d}  {:>8d}  {:>7.1f}ms  {:>6}  {:>6}  {:>5d}ms\n",
			elapsedSec,
			_num_connections.load(),
			_active_clients.load(),
			sent,
			avgRtt,
			minStr,
			maxStr,
			delayTime
		);
	}
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

	Disconnect(id);

	if (ex_over->_compType == CompType::SEND)
		delete ex_over;
}
