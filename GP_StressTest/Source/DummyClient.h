#pragma once
#include "ExpOver.h"

class DummyClient
{
public:
	DummyClient() {};
	~DummyClient() {};

	bool Init(uint32 num);
	bool Connect(IOCP& hIocp);
	bool Disconnect();
	void DoRecv();
	void DoSend(Packet* packet);

	void HandleRecvBuffer(int32 recvByte, ExpOver* expOver);
	void ProcessPacket(Packet* packet);

	void SendLoginPacket();
	void SendSignUpPacket();
	void SendRequestEnterGamePacket();
	bool SendMovePacket();
	bool Move();
	bool IsConnected() const
	{
		return _connected && _socket != INVALID_SOCKET;
	}
	bool IsLogin() const { return _logined; }
private:
	std::atomic_bool _connected = false;
	std::atomic_bool _logined = false;
	SOCKET _socket = INVALID_SOCKET;
	ExpOver _recvOver;
	int32 _remain = 0;
	FInfoData _info;
	uint32 _dummyNum = 0;
	std::string _name;
	int32& _playerId = _info.ID;
	high_resolution_clock::time_point last_move_time;
};

