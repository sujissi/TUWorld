#include "pch.h"
#include "DummyClient.h"

bool DummyClient::Init(uint32 num)
{
	_dummyNum = num;
	_name = std::format("Dummy{}", _dummyNum);
	return true;
}

bool DummyClient::Connect(IOCP& hIocp)
{
	if (_connected)
		return false;

	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_socket == INVALID_SOCKET)
		return false;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET,SERVER_IP, &addr.sin_addr);
	//inet_pton(AF_INET, "211.188.61.31", &addr.sin_addr);
	
	if (connect(_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		return false;
	}

	hIocp.RegisterSocket(_socket, _dummyNum);
	DoRecv();
	SendSignUpPacket();
	_connected = true;

	return true;
}

bool DummyClient::Disconnect()
{
	bool status = true;
	if (true == std::atomic_compare_exchange_strong(&_connected, &status, false)) {
		closesocket(_socket);
		return true;
	}
	return false;
}


void DummyClient::DoRecv()
{
	if (_socket == INVALID_SOCKET)
		return;
	ZeroMemory(&_recvOver._wsaover, sizeof(_recvOver._wsaover));
	DWORD recv_flag = 0;
	_recvOver._wsabuf.len = BUFSIZE - _remain;
	_recvOver._wsabuf.buf = reinterpret_cast<CHAR*>(_recvOver._buf) + _remain;
	WSARecv(_socket, &_recvOver._wsabuf, 1, 0, &recv_flag, &_recvOver._wsaover, 0);
}

void DummyClient::DoSend(Packet* packet)
{
	if (_socket == INVALID_SOCKET)
		return;
	auto send_data = new ExpOver{ packet };
	WSASend(_socket, &send_data->_wsabuf, 1, nullptr, 0, &send_data->_wsaover, nullptr);
}

void DummyClient::HandleRecvBuffer(int32 recvByte, ExpOver* expOver)
{
	int32 dataSize = recvByte + _remain;
	uint8* buf = expOver->_buf;
	int32 offset = 0;

	while (dataSize - offset >= sizeof(FPacketHeader))
	{
		FPacketHeader* header = reinterpret_cast<FPacketHeader*>(buf + offset);
		int32 packetSize = header->PacketSize;

		if (packetSize < sizeof(FPacketHeader) || packetSize > BUFSIZE)
		{
			break;
		}

		if (dataSize - offset < packetSize)
			break;

		Packet* packet = reinterpret_cast<Packet*>(buf + offset);
		ProcessPacket(packet);
		offset += packetSize;
	}

	_remain = dataSize - offset;
	if (_remain > 0)
		memmove(buf, buf + offset, _remain);
}


void DummyClient::ProcessPacket(Packet* packet)
{
	auto type = packet->Header.PacketType;
	switch (type)
	{
	case EPacketType::S_LOGIN_FAIL:
		SendSignUpPacket();
		break;
	case EPacketType::S_SIGNUP_FAIL:
		SendLoginPacket();
		break;
	case EPacketType::S_LOGIN_SUCCESS:
	case EPacketType::S_SIGNUP_SUCCESS:
	{
		SendRequestEnterGamePacket();
		break;
	}
	case EPacketType::S_ENTER_GAME:
	{
		auto pkt = reinterpret_cast<EnterGamePacket*>(packet);
		_info = pkt->PlayerInfo;
		_logined = true;
		_active_clients++;
		break;
	}
	case EPacketType::S_PLAYER_MOVE:
	{
		auto pkt = reinterpret_cast<MovePacket*>(packet);
		auto rtt_ms = NowMs() - pkt->MoveTime;
		UpdateDelay(static_cast<int>(rtt_ms));
		g_stats.RecordRtt(rtt_ms);
		break;
	}
	default:
		break;
	}
}

void DummyClient::SendLoginPacket()
{
	const auto accountID = std::format("{:05}", _dummyNum);
	const auto accountPW = "123";
	auto pkt = LoginPacket(accountID.c_str(), accountPW);
	DoSend(&pkt);
}

void DummyClient::SendSignUpPacket()
{
	const auto accountID = std::format("{:05}", _dummyNum);
	const auto accountPW = "123";
	auto pkt = SignUpPacket(accountID.c_str(), accountPW, _name.c_str());
	DoSend(&pkt);
}

void DummyClient::SendRequestEnterGamePacket()
{
	auto pkt = RequestEnterGamePacket(EWorldChannel::None);
	DoSend(&pkt);
}

bool DummyClient::SendMovePacket()
{
	if (last_move_time + 1s > high_resolution_clock::now()) return false;
	if (!Move()) 
	{
		TimerQueue::AddTimer([this]() {
			SendMovePacket();
			}, 1000, false);
		return false;
	}
	auto now = NowMs();
	_info.AddState(ECharacterStateType::STATE_WALK);
	auto sendPos = _info.Pos;
	MovePacket pkt(_playerId, sendPos, _info.State, now);
	DoSend(&pkt);
	++g_stats.moveSent;
	last_move_time = high_resolution_clock::now();
	return true;
}

bool DummyClient::Move()
{
	static auto& nav = Map::GetInst().GetNavMesh(ZoneType::TUK);
	FVector currentPos = _info.Pos;
	currentPos.Z -= 90.f;
	FVector newPos = nav.GetNearbyRandomPosition(currentPos);
	newPos.Z += 90.f;
	if (newPos == _info.Pos)
		return false;
	_info.SetLocationAndYaw(newPos);
	return true;
}
