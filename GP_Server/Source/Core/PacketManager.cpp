#include "pch.h"
#include "PacketManager.h"
#include "magic_enum/magic_enum.hpp"

void PacketManager::ProcessPacket(int32 sessionId, Packet* packet)
{
	EPacketType packetType = static_cast<EPacketType>(packet->Header.PacketType);
	auto name = std::string(magic_enum::enum_name(packetType));
	if (name.empty()) name = "Unknown";

	LOG_D("{} from [{}]", name, sessionId);

	switch (packetType)
	{
	case EPacketType::C_SIGNUP:
		HandleSignUpPacket(sessionId, packet);
		break;
	case EPacketType::C_LOGIN:
		HandleLoginPacket(sessionId, packet);
		break;
	case EPacketType::C_LOGOUT:
		HandleLogoutPacket(sessionId);
		break;
	case EPacketType::C_SELECT_CHARACTER:
		HandleSelectCharacterPacket(sessionId, packet);
		break;
	case EPacketType::C_ENTER_GAME:
		HandleEnterGamePacket(sessionId, packet);
		break;

	case EPacketType::C_MOVE:
		HandleMovePacket(sessionId, packet);
		break;
	case EPacketType::C_ATTACK:
		HandleAttackPacket(sessionId, packet);
		break;
	case EPacketType::C_START_AIMING:
		HandleStartAimingPacket(sessionId, packet);
		break;
	case EPacketType::C_STOP_AIMING:
		HandleStopAimingPacket(sessionId, packet);
		break;

	case EPacketType::C_USE_SKILL_START:
		HandleUseSkillPacket(sessionId, packet);
		break;
	case EPacketType::C_USE_SKILL_END:
		HandleEndSkillPacket(sessionId, packet);
		break;
	case EPacketType::C_REMOVE_STATE:
		HandleRemoveStatePacket(sessionId, packet);
		break;


	case EPacketType::C_TAKE_ITEM:
		HandlePickUpItemPacket(sessionId, packet);
		break;
	case EPacketType::C_DROP_ITEM:
		HandleDropItemPacket(sessionId, packet);
		break;
	case EPacketType::C_USE_ITEM:
		HandleUseItemPacket(sessionId, packet);
		break;
	case EPacketType::C_EQUIP_ITEM:
		HandleEquipItemPacket(sessionId, packet);
		break;
	case EPacketType::C_UNEQUIP_ITEM:
		HandleUnequipItemPacket(sessionId, packet);
		break;


	case EPacketType::C_CHANGE_ZONE:
		HandleZoneChangeRequestPacket(sessionId, packet);
		break;
	case EPacketType::C_RESPAWN:
		HandleRespawnRequestPacket(sessionId, packet);
		break;

	case EPacketType::C_SHOP_BUY_ITEM:
		HandleShopBuyItemPacket(sessionId, packet);
		break;
	case EPacketType::C_SHOP_SELL_ITEM:
		HandleShopSellItemPacket(sessionId, packet);
		break;


	case EPacketType::C_REQUEST_QUEST:
		HandleRequestQuestPacket(sessionId, packet);
		break;
	case EPacketType::C_COMPLETE_QUEST:
		HandleCompleteQuestPacket(sessionId, packet);
		break;
	case EPacketType::C_REJECT_QUEST:
		HandleRejectQuestPacket(sessionId, packet);
		break;

	case EPacketType::C_CHAT_SEND:
		HandleChatSendPacket(sessionId, packet);
		break;
	case EPacketType::C_CHAT_WHISPER:
		HandleChatWhisperPacket(sessionId, packet);
		break;


	case EPacketType::C_FRIEND_REQUEST:
		HandleFriendAddRequestPacket(sessionId, packet);
		break;
	case EPacketType::C_FRIEND_ACCEPT:
		HandleFriendAcceptRequestPacket(sessionId, packet);
		break;
	case EPacketType::C_FRIEND_REMOVE:
		HandleFriendRemoveRequestPacket(sessionId, packet);
		break;
	case EPacketType::C_FRIEND_REJECT:
		HandleFriendRejectRequestPacket(sessionId, packet);
		break;

	case EPacketType::C_CHANGE_CHANNEL:
		HandleChangeChannelPacket(sessionId, packet);
		break;
	}
}

void PacketManager::HandleSignUpPacket(int32 sessionId, Packet* packet)
{
	auto pkt = static_cast<SignUpPacket*>(packet);
#ifdef DB_MODE
	std::wstring name = ConvertToWString(pkt->NickName);
	std::string id = pkt->AccountID;
	std::string pw = pkt->AccountPW;

	DBWorker::GetInst().Push([sessionId, id, pw, name]() {
		auto res = DBManager::GetInst().SignUpUser(sessionId, id, pw, name);
		SessionManager::GetInst().Schedule(sessionId, [sessionId, res]() {
			if (res.code != ResultCode::SUCCESS)
			{
				LOG_D("SignUp Failed [{}]", sessionId);
				SignUpFailPacket failpkt(res.code);
				SessionManager::GetInst().SendPacket(sessionId, &failpkt);
				return;
			}
			SessionManager::GetInst().HandleLogin(sessionId, res);
			auto states = GameWorldManager::GetInst().GetAllWorldStates();
			SignUpSuccessPacket spkt(states.data());
			SessionManager::GetInst().SendPacket(sessionId, &spkt);
			LOG_D("SignUp Success [{}] userId: {}", sessionId, res.dbId);
		});
	});
	return;
#else
	LOG_D("ID: {}, PW: {}", pkt->AccountID, pkt->AccountPW);
	_sessionMgr.HandleLogin(sessionId);
	auto states = GameWorldManager::GetInst().GetAllWorldStates();
	LoginSuccessPacket loginpkt(states.data());
	_sessionMgr.SendPacket(sessionId, &loginpkt);
#endif
}

void PacketManager::HandleLoginPacket(int32 sessionId, Packet* packet)
{
	auto pkt = static_cast<LoginPacket*>(packet);
#ifdef DB_MODE
	std::string accountID = pkt->AccountID;
	std::string accountPW = pkt->AccountPW;

	DBWorker::GetInst().Push([sessionId, accountID, accountPW]() {
		auto res = DBManager::GetInst().CheckLogin(sessionId, accountID, accountPW);
		auto loginsess = SessionManager::GetInst().GetOnlineSessionIdByDBId(res.dbId);
		if (loginsess != -1 && loginsess != sessionId)
		{
			LOG_W("User already logged in [{}] - Disconnecting old session: {}", res.dbId, loginsess);
			LoginFailPacket failpkt(ResultCode::ALREADY_LOGGED_IN);
			SessionManager::GetInst().SendPacket(sessionId, &failpkt);
			return;
		}

		SessionManager::GetInst().Schedule(sessionId, [sessionId, res]() {
			if (res.code != ResultCode::SUCCESS)
			{
				LoginFailPacket failpkt(res.code);
				SessionManager::GetInst().SendPacket(sessionId, &failpkt);
				return;
			}
			SessionManager::GetInst().HandleLogin(sessionId, res);
			auto states = GameWorldManager::GetInst().GetAllWorldStates();
			LoginSuccessPacket loginpkt(states.data());
			SessionManager::GetInst().SendPacket(sessionId, &loginpkt);
			LOG_D("Login Success [{}] userId: {}", sessionId, res.dbId);
		});
	});
	return;

#else
	LOG_D("ID: {}, PW: {}", pkt->AccountID, pkt->AccountPW);
	_sessionMgr.HandleLogin(sessionId);
	auto states = GameWorldManager::GetInst().GetAllWorldStates();
	LoginSuccessPacket loginpkt(states.data());
	_sessionMgr.SendPacket(sessionId, &loginpkt);
#endif
}

void PacketManager::HandleLogoutPacket(int32 sessionId)
{
	auto pkt = IDPacket(EPacketType::S_REMOVE_PLAYER, sessionId);
	HandleWithWorld<IDPacket>(sessionId, &pkt, [this](GameWorld* world, IDPacket* p) {
		world->PlayerLeaveGame(p->Data);
		});
	_sessionMgr.Disconnect(sessionId);
}

void PacketManager::HandleSelectCharacterPacket(int32 sessionId, Packet* packet)
{
	SelectCharacterPacket* p = static_cast<SelectCharacterPacket*>(packet);
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsLogin()) return;

	auto player = session->GetPlayer();
	if (!player) return;

	player->SetCharacterType(p->PlayerType);

	if (session->IsInGame())
	{
		InfoPacket infopkt(EPacketType::S_PLAYER_STATUS_UPDATE, player->GetInfo());
		_sessionMgr.SendPacket(sessionId, &infopkt);
	}
}

void PacketManager::HandleEnterGamePacket(int32 sessionId, Packet* packet)
{
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsLogin()) return;

	auto player = session->GetPlayer();
	if (!player) return;
	RequestEnterGamePacket* p = static_cast<RequestEnterGamePacket*>(packet);
	auto newType = p->PlayerType;
	auto wChannelId = p->WChannel;
	GameWorld* world = nullptr;
	if(wChannelId == EWorldChannel::None)
	{
		world = GameWorldManager::GetInst().GetAvailableWorld(wChannelId);
	}
	else
	{
		world = GameWorldManager::GetInst().GetWorld(wChannelId);
	}

	if (!world)
	{
		LOG_W("Invalid World");
		return;
	}
	player->Init(wChannelId);
	if (newType != Type::EPlayer::NONE)
		player->SetCharacterType(p->PlayerType);
	session->EnterGame(wChannelId);
	world->PlayerEnterGame(player);
	uint32 dbId = session->GetDBID();
	DBWorker::GetInst().Push([sessionId, dbId]() {
		auto [friendResultCode, friendList] = DBManager::GetInst().GetFriendList(dbId);
		if (friendResultCode != ResultCode::SUCCESS)
			LOG_W("Failed to load friend list for user: {}", dbId);
		SessionManager::GetInst().Schedule(sessionId, [sessionId, friendList = std::move(friendList)]() {
			auto sess = SessionManager::GetInst().GetSession(sessionId);
			if (sess) sess->SetAndSendFriendsInfo(friendList);
		});
	});
}

void PacketManager::HandleMovePacket(int32 sessionId, Packet* packet)
{
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsInGame()) return;
	HandleWithWorld<MovePacket>(sessionId, packet, [](GameWorld* world, MovePacket* p) {
		world->PlayerMove(p->PlayerID, p->PlayerPos, p->State, p->MoveTime);
		});
}

void PacketManager::HandleStartAimingPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<StartAimingPacket>(sessionId, packet, [sessionId](GameWorld* world, StartAimingPacket* p) {
		world->PlayerSetLocation(sessionId, p->PlayerYaw, p->PlayerPos);
		world->PlayerAddState(sessionId, ECharacterStateType::STATE_AIMING);
		});
}

void PacketManager::HandleStopAimingPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<StopAimingPacket>(sessionId, packet, [sessionId](GameWorld* world, StopAimingPacket* p) {
		world->PlayerRemoveState(sessionId, ECharacterStateType::STATE_AIMING);
		});
}

void PacketManager::HandleAttackPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<AttackPacket>(sessionId, packet, [sessionId](GameWorld* world, AttackPacket* p) {
		world->PlayerSetLocation(sessionId, p->PlayerYaw, p->PlayerPos);
		world->PlayerAddState(sessionId, ECharacterStateType::STATE_AUTOATTACK);
		world->PlayerAttack(sessionId);
		});
}

void PacketManager::HandleUseSkillPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<UseSkillStartPacket>(sessionId, packet, [sessionId](GameWorld* world, UseSkillStartPacket* p) {
		world->PlayerSetLocation(sessionId, p->PlayerYaw, p->PlayerPos);
		world->PlayerUseSkill(sessionId, p->SkillGID);
		});
}

void PacketManager::HandleEndSkillPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<UseSkillEndPacket>(sessionId, packet, [sessionId](GameWorld* world, UseSkillEndPacket* p) {
		world->PlayerEndSkill(sessionId, p->SkillGID);
		});
}

void PacketManager::HandleRemoveStatePacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<RemoveStatePacket>(sessionId, packet, [sessionId](GameWorld* world, RemoveStatePacket* p) {
		world->PlayerRemoveState(sessionId, p->State);
		});
}

void PacketManager::HandlePickUpItemPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<IDPacket>(sessionId, packet, [sessionId](GameWorld* world, IDPacket* p) {
		world->PickUpWorldItem(sessionId, p->Data);
		});
}

void PacketManager::HandleDropItemPacket(int32 sessionId, Packet* packet)
{
}

void PacketManager::HandleUseItemPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<IDPacket>(sessionId, packet, [sessionId](GameWorld* world, IDPacket* p) {
		world->UseInventoryItem(sessionId, p->Data);
		});
}

void PacketManager::HandleEquipItemPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<IDPacket>(sessionId, packet, [sessionId](GameWorld* world, IDPacket* p) {
		world->EquipInventoryItem(sessionId, p->Data);
		});
}

void PacketManager::HandleUnequipItemPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<IDPacket>(sessionId, packet, [sessionId](GameWorld* world, IDPacket* p) {
		world->UnequipInventoryItem(sessionId, p->Data);
		});
}

void PacketManager::HandleZoneChangeRequestPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<RequestZoneChangePacket>(sessionId, packet, [this, sessionId](GameWorld* world, RequestZoneChangePacket* p) {
		world->TransferToZone(sessionId, p->TargetZone);
		});
}

void PacketManager::HandleRespawnRequestPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<RespawnRequestPacket>(sessionId, packet, [this, sessionId](GameWorld* world, RespawnRequestPacket* p) {
		world->RespawnPlayer(sessionId, p->TargetZone);
		});
}

void PacketManager::HandleShopBuyItemPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<BuyItemPacket>(sessionId, packet, [this, sessionId](GameWorld* world, BuyItemPacket* p) {
		world->BuyItem(sessionId, p->ItemType, p->Quantity);
		});
}

void PacketManager::HandleShopSellItemPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<SellItemPacket>(sessionId, packet, [this, sessionId](GameWorld* world, SellItemPacket* p) {
		world->SellItem(sessionId, p->ItemID);
		});
}

void PacketManager::HandleRequestQuestPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<RequestQuestPacket>(sessionId, packet, [this, sessionId](GameWorld* world, RequestQuestPacket* p) {
		world->RequestQuest(sessionId, p->Quest);
		});
}

void PacketManager::HandleCompleteQuestPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<CompleteQuestPacket>(sessionId, packet, [this, sessionId](GameWorld* world, CompleteQuestPacket* p) {
		world->CompleteQuest(sessionId, p->Quest, p->bForce);
		});
}

void PacketManager::HandleRejectQuestPacket(int32 sessionId, Packet* packet)
{
	HandleWithWorld<RejectQuestPacket>(sessionId, packet, [this, sessionId](GameWorld* world, RejectQuestPacket* p) {
		world->RejectQuest(sessionId, p->Quest);
		});
}

void PacketManager::HandleChatSendPacket(int32 sessionId, Packet* packet)
{
	auto* p = static_cast<ChatSendPacket*>(packet);
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsInGame()) return;

	auto player = session->GetPlayer();
	if (!player) return;

	const char* nickname = player->GetInfo().NickName;
	auto channel = p->Channel;
	switch (channel)
	{
	case EChatChannel::All:
	{
		ChatBroadcastPacket broadcastPkt(nickname, p->Message, channel);
		_sessionMgr.BroadcastToAll(&broadcastPkt);
		break;
	}
	case EChatChannel::Friend:
	{
		ChatBroadcastPacket broadcastPkt(nickname, p->Message, channel);
		_sessionMgr.SendPacket(sessionId, &broadcastPkt);
		_sessionMgr.BroadcastToFriends(sessionId, &broadcastPkt);
		break;
	}
	case EChatChannel::Zone:
	{
		ZoneType zone = player->GetZone();
		ChatBroadcastPacket broadcastPkt(nickname, p->Message, channel);
		if (auto world = GetValidWorld(sessionId))
			world->BroadcastToZone(zone, &broadcastPkt);
		break;
	}
	default:
		break;
	}
}

void PacketManager::HandleChatWhisperPacket(int32 sessionId, Packet* packet)
{
	auto* p = static_cast<ChatWhisperPacket*>(packet);
	auto sender = _sessionMgr.GetSession(sessionId);
	if (!sender || !sender->IsInGame()) return;

	if (sender->GetNickName() == p->TargetNickName)
	{
		FriendOperationResultPacket resPkt(EFriendOpType::Request, ResultCode::SELF_REQUEST);
		_sessionMgr.SendPacket(sessionId, &resPkt);
		return;
	}

	int32 targetId = _sessionMgr.FindSessionIdByName(p->TargetNickName);
	auto receiver = _sessionMgr.GetSession(targetId);
	if (!receiver || !receiver->IsInGame())
	{
		FriendOperationResultPacket resPkt(EFriendOpType::Request, ResultCode::TARGET_NOT_IN_GAME);
		_sessionMgr.SendPacket(sessionId, &resPkt);
		return;
	}

	EChatChannel channel = EChatChannel::Whisper;
	auto senderName = sender->GetPlayerInfo().NickName;
	ChatBroadcastPacket pkt(senderName, p->Message, channel);

	_sessionMgr.SendPacket(sessionId, &pkt);
	_sessionMgr.SendPacket(targetId, &pkt);
}


void PacketManager::HandleFriendAddRequestPacket(int32 sessionId, Packet* packet)
{
	auto* p = static_cast<FriendAddRequestPacket*>(packet);
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsLogin()) return;

	auto myId = session->GetDBID();
	auto targetNick = ConvertToWString(p->TargetNickName);
	auto& pInfo = session->GetPlayerInfo();
	std::wstring myName = ConvertToWString(pInfo.GetName());
	uint32 myLevel = pInfo.GetLevel();

	DBWorker::GetInst().Push([sessionId, myId, targetNick, myName, myLevel]() {
		int32 targetId = DBManager::GetInst().FindUserDBId(targetNick);
		ResultCode resCode = DBManager::GetInst().AddFriendRequest(myId, static_cast<uint32>(targetId));

		FriendOperationResultPacket resPkt(EFriendOpType::Request, resCode);
		SessionManager::GetInst().SendPacket(sessionId, &resPkt);
		if (resCode != ResultCode::SUCCESS) return;

		int32 targetSessId = SessionManager::GetInst().GetOnlineSessionIdByDBId(static_cast<uint32>(targetId));
		if (targetSessId != -1)
		{
			FFriendInfo info;
			info.DBId = myId;
			info.SetName(myName);
			info.Level = myLevel;
			info.isOnline = true;
			FriendRequestPacket requestPkt(info);
			SessionManager::GetInst().SendPacket(targetSessId, &requestPkt);
		}
	});
}

void PacketManager::HandleFriendRemoveRequestPacket(int32 sessionId, Packet* packet)
{
	auto* p = static_cast<FriendRemoveRequestPacket*>(packet);
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsLogin()) return;

	auto myId = session->GetDBID();
	auto targetId = p->TargetDBID;

	DBWorker::GetInst().Push([sessionId, myId, targetId]() {
		auto ret = DBManager::GetInst().RemoveFriend(myId, targetId);

		FriendOperationResultPacket resPkt(EFriendOpType::Remove, ret);
		SessionManager::GetInst().SendPacket(sessionId, &resPkt);
		if (ret != ResultCode::SUCCESS) return;

		SessionManager::GetInst().Schedule(sessionId, [sessionId, myId, targetId]() {
			auto sess = SessionManager::GetInst().GetSession(sessionId);
			if (!sess) return;
			sess->RemoveFriend(targetId);
			RemoveFriendPacket myPkt(targetId);
			SessionManager::GetInst().SendPacket(sessionId, &myPkt);

			int32 targetSessId = SessionManager::GetInst().GetOnlineSessionIdByDBId(targetId);
			if (targetSessId != -1)
			{
				auto targetSession = SessionManager::GetInst().GetSession(targetSessId);
				if (targetSession)
				{
					targetSession->RemoveFriend(myId);
					RemoveFriendPacket targetPkt(myId);
					SessionManager::GetInst().SendPacket(targetSessId, &targetPkt);
				}
			}
		});
	});
}

void PacketManager::HandleFriendAcceptRequestPacket(int32 sessionId, Packet* packet)
{
	auto* p = static_cast<FriendAcceptRequestPacket*>(packet);
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsLogin()) return;

	auto myId = session->GetDBID();
	auto targetId = p->TargetDBID;
	auto& selfInfo = session->GetPlayerInfo();
	std::wstring myName = ConvertToWString(selfInfo.GetName());
	uint32 myLevel = selfInfo.Stats.Level;

	DBWorker::GetInst().Push([sessionId, myId, targetId, myName, myLevel]() {
		auto ret = DBManager::GetInst().AcceptFriendRequest(myId, targetId);
		ResultCode code = ret.first;
		std::optional<FFriendInfo> friendInfoOpt = ret.second;

		FriendOperationResultPacket resPkt(EFriendOpType::Accept, code);
		SessionManager::GetInst().SendPacket(sessionId, &resPkt);
		if (code != ResultCode::SUCCESS || !friendInfoOpt.has_value())
			return;

		FFriendInfo friendInfo = friendInfoOpt.value();
		friendInfo.bAccepted = true;

		SessionManager::GetInst().Schedule(sessionId, [sessionId, myId, targetId, friendInfo, myName, myLevel]() {
			auto sess = SessionManager::GetInst().GetSession(sessionId);
			if (!sess) return;
			sess->AddFriend(friendInfo);
			AddFriendPacket myPkt(friendInfo);
			SessionManager::GetInst().SendPacket(sessionId, &myPkt);

			int32 targetSessId = SessionManager::GetInst().GetOnlineSessionIdByDBId(targetId);
			if (targetSessId != -1)
			{
				auto targetSession = SessionManager::GetInst().GetSession(targetSessId);
				if (targetSession)
				{
					FFriendInfo myInfo;
					myInfo.DBId = myId;
					myInfo.SetName(myName);
					myInfo.Level = myLevel;
					myInfo.bAccepted = true;
					targetSession->AddFriend(myInfo);
					AddFriendPacket targetPkt(myInfo);
					SessionManager::GetInst().SendPacket(targetSessId, &targetPkt);
				}
			}
		});
	});
}

void PacketManager::HandleFriendRejectRequestPacket(int32 sessionId, Packet* packet)
{
	auto* p = static_cast<FriendRejectRequestPacket*>(packet);
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsLogin()) return;

	auto myId = session->GetDBID();
	auto targetId = p->TargetDBID;

	DBWorker::GetInst().Push([sessionId, myId, targetId]() {
		auto ret = DBManager::GetInst().RejectFriendRequest(myId, targetId);

		FriendOperationResultPacket resPkt(EFriendOpType::Reject, ret);
		SessionManager::GetInst().SendPacket(sessionId, &resPkt);

		int32 targetSessId = SessionManager::GetInst().GetOnlineSessionIdByDBId(targetId);
		if (targetSessId != -1)
		{
			FriendOperationResultPacket targetResPkt(EFriendOpType::Reject, ret);
			SessionManager::GetInst().SendPacket(targetSessId, &targetResPkt);
		}
	});
}

void PacketManager::HandleChangeChannelPacket(int32 sessionId, Packet* packet)
{
	auto* p = static_cast<ChangeChannelRequestPacket*>(packet);
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsInGame()) return;
	auto world = GetValidWorld(sessionId);
	if (!world) return;
	if (world->GetChannelId() == p->NewChannel)
	{
		LOG_W("Already in channel {}", ENUM_NAME(p->NewChannel));
		return;
	}
	world->PlayerLeaveGame(sessionId);
	
	world = GameWorldManager::GetInst().GetWorld(p->NewChannel);
	if (!world)
	{
		LOG_W("Invalid World for channel {}", ENUM_NAME(p->NewChannel));
		return;
	}
	session->EnterGame(p->NewChannel);
	world->PlayerEnterGame(session->GetPlayer(), true);
}

GameWorld* PacketManager::GetValidWorld(int32 sessionId)
{
	auto session = _sessionMgr.GetSession(sessionId);
	if (!session || !session->IsInGame())
		return nullptr;

	auto world = GameWorldManager::GetInst().GetWorld(session->GetWorldChannel());
	if (!world)
		LOG_W("[PacketManager] Invalid world for sessionId: {}", sessionId);

	return world;
}