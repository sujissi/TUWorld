#include "pch.h"
#include "PlayerSession.h"
#include "SessionManager.h"
#include "GameWorld.h"
#include "PacketManager.h"

void PlayerSession::DoRecv()
{
	_sSocket->DoRecv();
}

void PlayerSession::DoSend(const Packet* packet)
{
	if (packet->Header.PacketType != EPacketType::S_MONSTER_STATUS_UPDATE)
	{
		auto name = ENUM_NAME(static_cast<EPacketType>(packet->Header.PacketType));
		LOG_D(">>> {} PKT to [{}]", name, _id);
	}

	_sSocket->DoSend(packet);
}

void PlayerSession::OnRecv(int32 recvByte, ExpOver* expOver)
{
	_sSocket->OnRecv(_id, recvByte, expOver);
}

void PlayerSession::Connect(SOCKET socket, int32 id)
{
	this->_id = id;
	_sSocket = std::make_unique<SessionSocket>(socket);
}

void PlayerSession::Disconnect()
{
	Logout();
	_sSocket->Disconnect();
	_sSocket.reset();
}

void PlayerSession::Login(const DBLoginResult& dbRes)
{
	_state = SessionState::LoggedIn;
	_player = std::make_shared<Player>(_id);
#ifdef DB_MODE
	{
		SetDBID(dbRes.dbId);
		SetNickName(dbRes.info.GetName());
		_player->LoadFromDB(dbRes);
	}
#endif
}

void PlayerSession::EnterGame(EWorldChannel wchannel)
{
	_wChannel = wchannel;
	_state = SessionState::InGame;
	_player->SetWorldChannel(wchannel);
}

void PlayerSession::Logout()
{
	if (IsLogin())
	{
		//todo: 뷰리스트 제거
		_state = SessionState::None;
#ifdef DB_MODE
		uint32 dbId = _dbId;
		FInfoData info = _player->GetInfo();
		std::vector<uint8> newItems = _player->GetUnsavedItemTypes();
		_player.reset();

		DBWorker::GetInst().Push([dbId, info = std::move(info), newItems = std::move(newItems)]() {
			DBManager::GetInst().SavePlayer(dbId, info, newItems);
		});
#endif
	}
}

int32 PlayerSession::GetId()
{
	return _id;
}

FInfoData& PlayerSession::GetPlayerInfo()
{
	return _player->GetInfo();
}

void PlayerSession::SetAndSendFriendsInfo(std::vector<FFriendInfo> friends)
{
	_friends = std::move(friends);
	//게임입장 후 위젯 생성 이후로 보내줘야 함
	if (!_friends.empty())
	{
		const auto& friends = _friends;
		for (const FFriendInfo& f : friends)
		{
			if (f.bAccepted)
			{
				AddFriendPacket pkt(f);
				SessionManager::GetInst().SendPacket(_id, &pkt);
			}
			else if (!f.bIsRequester)
			{
				FriendRequestPacket requestPkt(f);
				SessionManager::GetInst().SendPacket(_id, &requestPkt);
			}
		}
	}
}

std::shared_ptr<Player> PlayerSession::GetPlayer()
{
	return _player;
}