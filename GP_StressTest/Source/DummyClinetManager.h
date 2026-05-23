#pragma once
#include "DummyClient.h"

constexpr int32 CLIENT_NUM = 5000;
class DummyClientManager
{
public:
	static DummyClientManager& GetInst()
	{
		static DummyClientManager inst;
		return inst;
	}
	bool Init();
	void Run();
	void Shutdown();
	void SendMovePacket(int i);

	void WorkerThread();
	void TestThread();
	void StatsThread();
	void HandleRecv(int32 id, DWORD rbyte, LPWSAOVERLAPPED over);
	void AdjustClientCount();
	void HandleCompletionError(ExpOver* ex_over, int32 id);
	void Connect(int32 id)
	{
		if (_clients[id].Connect(_hIocp))
			_num_connections++;
	}
	void Disconnect(int32 id)
	{
		if (_clients[id].Disconnect())
			_active_clients--;
	}
private:
	IOCP& _hIocp = IOCP::GetInst();
	std::array<DummyClient, CLIENT_NUM> _clients;
	high_resolution_clock::time_point last_connect_time{};

	std::atomic_int _num_connections;
	std::atomic_int _client_to_close;
	std::vector<std::thread> _threads;
	std::atomic<bool> _running{ true };
};

