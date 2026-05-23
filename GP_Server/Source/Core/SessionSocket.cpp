#include "pch.h"
#include "SessionSocket.h"
#include "PacketManager.h"

SessionSocket::SessionSocket(SOCKET socket)
	: _socket(socket) {
};

SessionSocket::~SessionSocket()
{
	Disconnect();
}

void SessionSocket::Init(SOCKET& socket)
{
	this->_socket = socket;
}

void SessionSocket::Disconnect()
{
	closesocket(_socket);
	_socket = INVALID_SOCKET;
}

void SessionSocket::DoRecv()
{
	if (_socket == INVALID_SOCKET)
		return;
	ZeroMemory(&_recvOver._wsaover, sizeof(_recvOver._wsaover));
	DWORD recv_flag = 0;
	_recvOver._wsabuf.len = BUFSIZE - _remain;
	_recvOver._wsabuf.buf = reinterpret_cast<CHAR*>(_recvOver._buf) + _remain;
	WSARecv(_socket, &_recvOver._wsabuf, 1, 0, &recv_flag, &_recvOver._wsaover, 0);
}

void SessionSocket::DoSend(const Packet* packet)
{
	if (_socket == INVALID_SOCKET)
		return;

	auto over = new ExpOver{ packet };
	int ret = WSASend(_socket, &over->_wsabuf, 1, nullptr, 0, &over->_wsaover, nullptr);
	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		delete over;
	}
}

void SessionSocket::OnRecv(int32 id, int32 recvByte, ExpOver* expOver)
{
	uint32 dataSize = recvByte + _remain;
	if (dataSize >= sizeof(FPacketHeader))
	{
		uint8* buffer = expOver->_buf;
		Packet* packet = reinterpret_cast<Packet*>(buffer);
		while (dataSize >= sizeof(FPacketHeader))
		{
			uint32 packetSize = packet->Header.PacketSize;
			if (packetSize <= 0 || packetSize > dataSize)
				break;

			auto name = ENUM_NAME(static_cast<EPacketType>(packet->Header.PacketType));
			LOG_D("<<< {} PKT from [{}]", name, id);
			auto sharedPacket = std::shared_ptr<Packet>(
				reinterpret_cast<Packet*>(new uint8[packet->Header.PacketSize]),
				[](Packet* p) { delete[] reinterpret_cast<uint8_t*>(p); }
			);
			memcpy(sharedPacket.get(), packet, packet->Header.PacketSize);
			SessionManager::GetInst().Schedule(id, [sharedPacket, id]() {
				PacketManager::GetInst().ProcessPacket(id, sharedPacket.get());
				});

			buffer += packetSize;
			dataSize -= packetSize;
			packet = reinterpret_cast<Packet*>(buffer);
		}
		if (dataSize > 0)
			memmove(expOver->_buf, buffer, dataSize);
	}

	_remain = dataSize;
}
