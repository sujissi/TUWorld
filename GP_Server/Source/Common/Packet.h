#pragma once
#include "Common.h"

#pragma pack(push,1)
struct Packet
{
	struct PacketHeader
	{
		EPacketType PacketType;
		int32 PacketSize;

		PacketHeader(EPacketType type, uint8_t size)
			: PacketType(type), PacketSize(size) {
		}
	};

	PacketHeader Header;

	Packet(EPacketType type)
		: Header(type, sizeof(PacketHeader)) {
	}
};

template<typename T>
struct TPacket : public Packet
{
	T Data;
	TPacket(EPacketType type, const T& payload)
		: Packet(type), Data(payload)
	{
		Header.PacketSize = sizeof(PacketHeader) + sizeof(T);
	}
};

using FPacketHeader = Packet::PacketHeader;
using InfoPacket = TPacket<FInfoData>;
using IDPacket = TPacket<int32>;

struct ChangedWorldStatePacket : public Packet
{
	FWorldState WorldState;

	ChangedWorldStatePacket(const FWorldState& InWorldStates)
		: Packet(EPacketType::S_WORLD_STATE), WorldState(InWorldStates)
	{
		Header.PacketSize = sizeof(ChangedWorldStatePacket);
	}
};
struct ChangeChannelRequestPacket :public Packet
{
	EWorldChannel NewChannel;
	ChangeChannelRequestPacket(EWorldChannel InWChannel)
		: Packet(EPacketType::C_CHANGE_CHANNEL), NewChannel(InWChannel)
	{
		Header.PacketSize = sizeof(ChangeChannelRequestPacket);
	}
};
struct ChangeChannelPacket : public Packet
{
	FVector PlayerPos;
	EWorldChannel WChannel;

	ChangeChannelPacket(const FVector& pos, EWorldChannel wChannel)
		: Packet(EPacketType::S_CHANGE_CHANNEL), PlayerPos(pos), WChannel(wChannel)
	{
		Header.PacketSize = sizeof(ChangeChannelPacket);
	}
};

#pragma region Account
struct LoginPacket : public Packet
{
	char AccountID[LOGIN_STR_LEN];
	char AccountPW[LOGIN_STR_LEN];
	LoginPacket(const char* AccountID_, const char* AccountPW_)
		: Packet(EPacketType::C_LOGIN)
	{
		SAFE_STRCPY(AccountID, AccountID_, LOGIN_STR_LEN - 1);
		AccountID[LOGIN_STR_LEN - 1] = '\0';

		SAFE_STRCPY(AccountPW, AccountPW_, LOGIN_STR_LEN - 1);
		AccountPW[LOGIN_STR_LEN - 1] = '\0';

		Header.PacketSize = sizeof(LoginPacket);
	}
};

struct SignUpPacket : public Packet
{
	char AccountID[LOGIN_STR_LEN];
	char AccountPW[LOGIN_STR_LEN];
	char NickName[NICKNAME_LEN];

	SignUpPacket(const char* InAccountID, const char* InAccountPW, const char* InNickName)
		: Packet(EPacketType::C_SIGNUP)
	{
		SAFE_STRCPY(AccountID, InAccountID, LOGIN_STR_LEN - 1);
		AccountID[LOGIN_STR_LEN - 1] = '\0';

		SAFE_STRCPY(AccountPW, InAccountPW, LOGIN_STR_LEN - 1);
		AccountPW[LOGIN_STR_LEN - 1] = '\0';

		SAFE_STRCPY(NickName, InNickName, NICKNAME_LEN - 1);
		NickName[NICKNAME_LEN - 1] = '\0';

		Header.PacketSize = sizeof(SignUpPacket);
	}
};

struct LoginSuccessPacket : public Packet
{
	FWorldState WorldState[WORLD_MAX_COUNT];

	LoginSuccessPacket(const FWorldState* InWorldStates)
		: Packet(EPacketType::S_LOGIN_SUCCESS)
	{
		for (int i = 0; i < WORLD_MAX_COUNT; ++i)
		{
			WorldState[i] = InWorldStates[i];
		}
		Header.PacketSize = sizeof(LoginSuccessPacket);
	}
};

struct LoginFailPacket : public Packet
{
	ResultCode ResCode;
	LoginFailPacket(const ResultCode resCode)
		: Packet(EPacketType::S_LOGIN_FAIL), ResCode(resCode)
	{
		Header.PacketSize = sizeof(LoginFailPacket);
	}
};

struct SignUpSuccessPacket : public Packet
{
	FWorldState WorldState[WORLD_MAX_COUNT];

	SignUpSuccessPacket(const FWorldState* InWorldStates)
		: Packet(EPacketType::S_SIGNUP_SUCCESS)
	{
		for (int i = 0; i < WORLD_MAX_COUNT; ++i)
		{
			WorldState[i] = InWorldStates[i];
		}
		Header.PacketSize = sizeof(SignUpSuccessPacket);
	}
};

struct SignUpFailPacket : public Packet
{
	ResultCode ResCode;
	SignUpFailPacket(const ResultCode resCode)
		: Packet(EPacketType::S_SIGNUP_FAIL), ResCode(resCode)
	{
		Header.PacketSize = sizeof(SignUpFailPacket);
	}
};
#pragma endregion

#pragma region Player
struct SelectCharacterPacket : public Packet
{
	Type::EPlayer PlayerType;
	SelectCharacterPacket(Type::EPlayer PlayerType_)
		: Packet(EPacketType::C_SELECT_CHARACTER), PlayerType(PlayerType_)
	{
		Header.PacketSize = sizeof(SelectCharacterPacket);
	}
};
struct RequestEnterGamePacket : public Packet
{
	EWorldChannel WChannel;
	Type::EPlayer PlayerType;
	RequestEnterGamePacket(EWorldChannel Channel = EWorldChannel::TUWorld_1, Type::EPlayer InType = Type::EPlayer::NONE)
		: Packet(EPacketType::C_ENTER_GAME), WChannel(Channel), PlayerType(InType)
	{
		Header.PacketSize = sizeof(RequestEnterGamePacket);
	}
};
struct EnterGamePacket : public Packet
{
	FInfoData PlayerInfo;
	EWorldChannel WChannel;

	EnterGamePacket(const FInfoData& PlayerInfo_, EWorldChannel wChannel)
		: Packet(EPacketType::S_ENTER_GAME), PlayerInfo(PlayerInfo_), WChannel(wChannel)
	{
		Header.PacketSize = sizeof(EnterGamePacket);
	}
};

struct MovePacket : public Packet
{
	int32 PlayerID;
	FVector PlayerPos;
	uint32 State;
	uint64 MoveTime;

	MovePacket(int32 PlayerID_, const FVector& PlayerPos_, uint32 State_,
		uint64 SendTime_ = 0, EPacketType Type_ = EPacketType::C_MOVE)
		: Packet(Type_), PlayerID(PlayerID_), PlayerPos(PlayerPos_), State(State_), MoveTime(SendTime_)
	{
		Header.PacketSize = sizeof(MovePacket);
	}
};
struct PlayerLevelUpPacket : public Packet
{
	FInfoData PlayerInfo;
	PlayerLevelUpPacket(const FInfoData& PlayerInfo_)
		: Packet(EPacketType::S_LEVEL_UP), PlayerInfo(PlayerInfo_)
	{
		Header.PacketSize = sizeof(PlayerLevelUpPacket);
	}
};
#pragma endregion

#pragma region Attack
struct AttackPacket : public Packet
{
	float PlayerYaw;
	FVector PlayerPos;
	AttackPacket(float yaw, FVector pos)
		: Packet(EPacketType::C_ATTACK), PlayerYaw(yaw), PlayerPos(pos)
	{
		Header.PacketSize = sizeof(AttackPacket);
	}
};
struct PlayerAttackPacket : public Packet
{
	int32 PlayerID;
	FVector PlayerPos;
	float PlayerYaw;
	PlayerAttackPacket(int32 playerID, FVector pos, float yaw)
		: Packet(EPacketType::S_PLAYER_ATTACK), PlayerID(playerID), PlayerPos(pos), PlayerYaw(yaw)
	{
		Header.PacketSize = sizeof(PlayerAttackPacket);
	}
};
struct StartAimingPacket : public Packet
{
	float PlayerYaw;
	FVector PlayerPos;
	StartAimingPacket(float PlayerYaw_, FVector PlayerPos_)
		: Packet(EPacketType::C_START_AIMING), PlayerYaw(PlayerYaw_), PlayerPos(PlayerPos_)
	{
		Header.PacketSize = sizeof(StartAimingPacket);
	}
};
struct StopAimingPacket : public Packet
{
	bool dummy;
	StopAimingPacket()
		: Packet(EPacketType::C_STOP_AIMING)
	{
		Header.PacketSize = sizeof(StopAimingPacket);
	}
};
struct MonsterDamagePacket : public Packet
{
	FInfoData Target;
	float Damage;

	MonsterDamagePacket(const FInfoData& Target_, float Damage_)
		: Packet(EPacketType::S_DAMAGED_MONSTER), Target(Target_), Damage(Damage_)
	{
		Header.PacketSize = sizeof(MonsterDamagePacket);
	}
};
struct PlayerDamagePacket : public Packet
{
	FInfoData Target;

	PlayerDamagePacket(const FInfoData& Target_)
		: Packet(EPacketType::S_DAMAGED_PLAYER), Target(Target_)
	{
		Header.PacketSize = sizeof(PlayerDamagePacket);
	}
};
struct MonsterDeadPacket : public Packet
{
	int32 MonsterID;

	MonsterDeadPacket(int32 MonsterID_)
		: Packet(EPacketType::S_MONSTER_DEAD), MonsterID(MonsterID_)
	{
		Header.PacketSize = sizeof(MonsterDeadPacket);
	}
};
#pragma endregion

#pragma region Skill
struct UseSkillStartPacket : public Packet
{
	ESkillGroup SkillGID;
	float PlayerYaw;
	FVector PlayerPos;

	UseSkillStartPacket(ESkillGroup SkillGID_, float PlayerYaw_, FVector PlayerPos_)
		: Packet(EPacketType::C_USE_SKILL_START), SkillGID(SkillGID_), PlayerYaw(PlayerYaw_), PlayerPos(PlayerPos_)
	{
		Header.PacketSize = sizeof(UseSkillStartPacket);
	}
};
struct UseSkillEndPacket : public Packet
{
	ESkillGroup SkillGID;

	UseSkillEndPacket(ESkillGroup SkillGID_)
		: Packet(EPacketType::C_USE_SKILL_END), SkillGID(SkillGID_)
	{
		Header.PacketSize = sizeof(UseSkillEndPacket);
	}
};
struct SkillUnlockPacket : public Packet
{
	ESkillGroup SkillGID;
	SkillUnlockPacket(ESkillGroup SkillGID_)
		: Packet(EPacketType::S_SKILL_UNLOCK), SkillGID(SkillGID_)
	{
		Header.PacketSize = sizeof(SkillUnlockPacket);
	}
};
struct UpgradeSkillPacket : public Packet
{
	ESkillGroup SkillGID;
	UpgradeSkillPacket(ESkillGroup SkillGID_)
		: Packet(EPacketType::S_SKILL_UPGRADE), SkillGID(SkillGID_)
	{
		Header.PacketSize = sizeof(UpgradeSkillPacket);
	}
};
struct PlayerUseSkillStartPacket : public Packet
{
	int32 PlayerID;
	ESkillGroup SkillGID;
	float PlayerYaw;
	FVector PlayerPos;

	PlayerUseSkillStartPacket(int32 PlayerID_, ESkillGroup SkillGID_, float PlayerYaw_, FVector PlayerPos_)
		: Packet(EPacketType::S_PLAYER_USE_SKILL_START), PlayerID(PlayerID_), SkillGID(SkillGID_), PlayerYaw(PlayerYaw_), PlayerPos(PlayerPos_)
	{
		Header.PacketSize = sizeof(PlayerUseSkillStartPacket);
	}
};
struct PlayerUseSkillEndPacket : public Packet
{
	int32 PlayerID;

	PlayerUseSkillEndPacket(int32 PlayerID_)
		: Packet(EPacketType::S_PLAYER_USE_SKILL_END), PlayerID(PlayerID_)
	{
		Header.PacketSize = sizeof(PlayerUseSkillEndPacket);
	}
};
struct PlayerDeadPacket : public Packet
{
	int32 PlayerID;

	PlayerDeadPacket(int32 playerId)
		: Packet(EPacketType::S_PLAYER_DEAD), PlayerID(playerId)
	{
		Header.PacketSize = sizeof(PlayerDeadPacket);
	}
};

struct RemoveStatePacket : public Packet
{
	ECharacterStateType State;

	RemoveStatePacket(ECharacterStateType State_)
		: Packet(EPacketType::C_REMOVE_STATE), State(State_)
	{
		Header.PacketSize = sizeof(RemoveStatePacket);
	}
};


#pragma endregion

#pragma region Item
namespace ItemPkt
{
	struct SpawnPacket : public Packet
	{
		uint32 ItemID;
		uint8 ItemType;
		FVector Pos;

		SpawnPacket(uint32 itemId, uint8 type, FVector pos)
			: Packet(EPacketType::S_ITEM_SPAWN), ItemID(itemId), ItemType(type), Pos(pos)
		{
			Header.PacketSize = sizeof(SpawnPacket);
		}
	};

	struct DespawnPacket : public Packet
	{
		uint32 ItemID;
		DespawnPacket(uint32 itemId)
			: Packet(EPacketType::S_ITEM_DESPAWN), ItemID(itemId)
		{
			Header.PacketSize = sizeof(DespawnPacket);
		}
	};

	struct PickUpPacket : public Packet
	{
		uint32 ItemID;
		PickUpPacket(uint32 itemId)
			: Packet(EPacketType::S_ITEM_PICKUP), ItemID(itemId)
		{
			Header.PacketSize = sizeof(PickUpPacket);
		}
	};

	struct DropPacket : public Packet
	{
		uint32 ItemID;
		uint8 ItemType;
		FVector Pos;

		DropPacket(uint32 itemId, uint8 type, FVector pos)
			: Packet(EPacketType::S_ITEM_SPAWN), ItemID(itemId), ItemType(type), Pos(pos)
		{
			Header.PacketSize = sizeof(DropPacket);
		}
	};

	struct AddInventoryPacket : public Packet
	{
		uint32 ItemID;
		uint8 ItemType;

		AddInventoryPacket(uint32 itemId, uint8 type)
			: Packet(EPacketType::S_ADD_INVENTORY_ITEM), ItemID(itemId), ItemType(type)
		{
			Header.PacketSize = sizeof(AddInventoryPacket);
		}
	};
	struct RemoveInventoryPacket : public Packet
	{
		uint32 ItemID;
		RemoveInventoryPacket(uint32 itemId)
			: Packet(EPacketType::S_REMOVE_INVENTORY_ITEM), ItemID(itemId)
		{
			Header.PacketSize = sizeof(RemoveInventoryPacket);
		}
	};
	struct ItemUsedPacket : public Packet
	{
		uint32 ItemID;
		FInfoData PlayerInfo;

		ItemUsedPacket(uint32 itemId, const FInfoData& PlayerInfo_)
			: Packet(EPacketType::S_USE_INVENTORY_ITEM), ItemID(itemId), PlayerInfo(PlayerInfo_)
		{
			Header.PacketSize = sizeof(ItemUsedPacket);
		}
	};

	struct EquipItemPacket : public Packet
	{
		int32 PlayerID;
		uint8 ItemType;
		FInfoData PlayerInfo;

		EquipItemPacket(int32 playerId, uint8 type, const FInfoData& PlayerInfo_)
			: Packet(EPacketType::S_EQUIP_ITEM), PlayerID(playerId), ItemType(type), PlayerInfo(PlayerInfo_)
		{
			Header.PacketSize = sizeof(EquipItemPacket);
		}
	};

	struct UnequipItemPacket : public Packet
	{
		int32 PlayerID;
		uint8 ItemType;
		FInfoData PlayerInfo;

		UnequipItemPacket(int32 playerId, uint8 type, const FInfoData& PlayerInfo_)
			: Packet(EPacketType::S_UNEQUIP_ITEM), PlayerID(playerId), ItemType(type), PlayerInfo(PlayerInfo_)
		{
			Header.PacketSize = sizeof(UnequipItemPacket);
		}
	};
}// namespace Item
#pragma endregion

#pragma region Zone
struct RequestZoneChangePacket : public Packet
{
	ZoneType TargetZone;

	RequestZoneChangePacket(ZoneType zone)
		: Packet(EPacketType::C_CHANGE_ZONE), TargetZone(zone)
	{
		Header.PacketSize = sizeof(RequestZoneChangePacket);
	}
};
struct ChangeZonePacket : public Packet
{
	ZoneType TargetZone;
	FVector RandomPos;

	ChangeZonePacket(ZoneType zone, const FVector& pos)
		: Packet(EPacketType::S_CHANGE_ZONE), TargetZone(zone), RandomPos(pos)
	{
		Header.PacketSize = sizeof(ChangeZonePacket);
	}
};
struct RespawnRequestPacket : public Packet
{
	ZoneType TargetZone;

	RespawnRequestPacket(ZoneType zone)
		: Packet(EPacketType::C_RESPAWN), TargetZone(zone)
	{
		Header.PacketSize = sizeof(RespawnRequestPacket);
	}
};
struct RespawnPacket : public Packet
{
	FInfoData PlayerInfo;

	RespawnPacket(const FInfoData& info)
		: Packet(EPacketType::S_RESPAWN), PlayerInfo(info)
	{
		Header.PacketSize = sizeof(RespawnPacket);
	}
};
#pragma endregion

#pragma region Shop
constexpr uint8 MAX_SHOP_ITEMS = 32;

struct ShopItemListPacket : public Packet
{
	uint8          ItemCount;
	ShopItemInfo   ShopItems[MAX_SHOP_ITEMS];

	ShopItemListPacket(uint8 count, const ShopItemInfo* items)
		: Packet(EPacketType::S_SHOP_ITEM_LIST)
		, ItemCount(count)
	{
		for (uint8 i = 0; i < count && i < MAX_SHOP_ITEMS; ++i)
			ShopItems[i] = items[i];

		Header.PacketSize = sizeof(ShopItemListPacket);
	}
};

struct BuyItemPacket : public Packet
{
	uint8 ItemType;
	uint16 Quantity;

	BuyItemPacket(uint8 itype, uint16 qty)
		: Packet(EPacketType::C_SHOP_BUY_ITEM)
		, ItemType(itype)
		, Quantity(qty)
	{
		Header.PacketSize = sizeof(BuyItemPacket);
	}
};

struct BuyItemResultPacket : public Packet
{
	bool          bSuccess;
	ResultCode		 ResCode;
	uint32        PlayerGold;

	BuyItemResultPacket(bool success, ResultCode code, uint32 updateGold)
		: Packet(EPacketType::S_SHOP_BUY_RESULT)
		, bSuccess(success)
		, ResCode(code)
		, PlayerGold(updateGold)
	{
		Header.PacketSize = sizeof(BuyItemResultPacket);
	}
};

struct SellItemPacket : public Packet
{
	uint32 ItemID;
	SellItemPacket(uint32 itemId)
		: Packet(EPacketType::C_SHOP_SELL_ITEM)
		, ItemID(itemId)
	{
		Header.PacketSize = sizeof(SellItemPacket);
	}
};

struct SellItemResultPacket : public Packet
{
	bool          bSuccess;
	ResultCode  ResCode;
	uint32        PlayerGold;

	SellItemResultPacket(bool success, ResultCode code, uint32 updateGold)
		: Packet(EPacketType::S_SHOP_SELL_RESULT)
		, bSuccess(success)
		, ResCode(code)
		, PlayerGold(updateGold)
	{
		Header.PacketSize = sizeof(SellItemResultPacket);
	}
};

#pragma endregion

#pragma region Quest

struct RequestQuestPacket : public Packet
{
	QuestType Quest;
	RequestQuestPacket(QuestType quest)
		: Packet(EPacketType::C_REQUEST_QUEST)
		, Quest(quest)
	{
		Header.PacketSize = sizeof(RequestQuestPacket);
	}
};

struct CompleteQuestPacket : public Packet
{
	QuestType Quest;
	bool bForce = false;
	CompleteQuestPacket(QuestType quest, bool force)
		: Packet(EPacketType::C_COMPLETE_QUEST), Quest(quest), bForce(force)
	{
		Header.PacketSize = sizeof(CompleteQuestPacket);
	}
};

struct RejectQuestPacket : public Packet
{
	QuestType Quest;
	RejectQuestPacket(QuestType quest)
		: Packet(EPacketType::C_REJECT_QUEST)
		, Quest(quest)
	{
		Header.PacketSize = sizeof(RejectQuestPacket);
	}
};

struct QuestStartPacket : public Packet
{
	QuestType Quest;
	QuestStartPacket(QuestType quest)
		: Packet(EPacketType::S_QUEST_START)
		, Quest(quest)
	{
		Header.PacketSize = sizeof(QuestStartPacket);
	}
};

struct QuestRewardPacket : public Packet
{
	QuestType Quest;
	bool   bSuccess;
	uint32 ExpReward;
	uint32 GoldReward;

	QuestRewardPacket(QuestType quest, bool ok, uint32 exp, uint32 gold)
		: Packet(EPacketType::S_QUEST_REWARD)
		, Quest(quest)
		, bSuccess(ok)
		, ExpReward(exp)
		, GoldReward(gold)
	{
		Header.PacketSize = sizeof(QuestRewardPacket);
	}
};

#pragma endregion

#pragma region Chat

struct ChatSendPacket : public Packet
{
	char Message[CHAT_MESSAGE_LEN];
	EChatChannel Channel;

	ChatSendPacket(const char* Msg, EChatChannel channel)
		: Packet(EPacketType::C_CHAT_SEND), Channel(channel)
	{
		SAFE_STRCPY(Message, Msg, CHAT_MESSAGE_LEN - 1);
		Message[CHAT_MESSAGE_LEN - 1] = '\0';

		Header.PacketSize = sizeof(ChatSendPacket);
	}
};

struct ChatWhisperPacket : public Packet
{
	char TargetNickName[NICKNAME_LEN];
	char Message[CHAT_MESSAGE_LEN];

	ChatWhisperPacket(const char* Name, const char* Msg)
		: Packet(EPacketType::C_CHAT_WHISPER)
	{
		SAFE_STRCPY(TargetNickName, Name, NICKNAME_LEN - 1);
		TargetNickName[NICKNAME_LEN - 1] = '\0';

		SAFE_STRCPY(Message, Msg, CHAT_MESSAGE_LEN - 1);
		Message[CHAT_MESSAGE_LEN - 1] = '\0';

		Header.PacketSize = sizeof(ChatWhisperPacket);
	}
};

struct ChatBroadcastPacket : public Packet
{
	char SenderNickName[NICKNAME_LEN];
	char Message[CHAT_MESSAGE_LEN];
	EChatChannel Channel;
	ChatBroadcastPacket(const char* Sender, const char* Msg, EChatChannel channel)
		: Packet(EPacketType::S_CHAT_BROADCAST), Channel(channel)
	{
		SAFE_STRCPY(SenderNickName, Sender, NICKNAME_LEN - 1);
		SenderNickName[NICKNAME_LEN - 1] = '\0';

		SAFE_STRCPY(Message, Msg, CHAT_MESSAGE_LEN - 1);
		Message[CHAT_MESSAGE_LEN - 1] = '\0';

		Header.PacketSize = sizeof(ChatBroadcastPacket);
	}
};

#pragma endregion

#pragma region Boss
namespace Tino
{
	struct EarthQuakePacket : public Packet
	{
		FVector RockPos;
		bool bDebug = false;
		EarthQuakePacket(const FVector& rockPos, bool debug = false)
			:Packet(EPacketType::S_EARTH_QUAKE), RockPos(rockPos), bDebug(debug)
		{
			Header.PacketSize = sizeof(EarthQuakePacket);
		}
	};

	struct FlameBreathPacket : public Packet
	{
		FVector Origin;
		FVector Direction;
		float Range;
		float AngleDeg;
		bool bDebug = false;

		FlameBreathPacket(const FVector& origin, const FVector& direction, float range, float angle, bool debug = false)
			:Packet(EPacketType::S_FLAME_BREATH),
			Origin(origin), Direction(direction), Range(range), AngleDeg(angle), bDebug(debug)
		{
			Header.PacketSize = sizeof(FlameBreathPacket);
		}
	};

}
#pragma endregion

#pragma region Debug //for test
struct DebugTrianglePacket : public Packet
{
	FVector A;
	FVector B;
	FVector C;
	float  Duration;

	DebugTrianglePacket(const FVector& InA, const FVector& InB, const FVector& InC, float InDuration = 10.0f)
		: Packet(EPacketType::S_DEBUG_TRIANGLE)
		, A(InA), B(InB), C(InC), Duration(InDuration)
	{
		Header.PacketSize = sizeof(DebugTrianglePacket);
	}
};

struct DebugLinePacket : public Packet
{
	FVector A;
	FVector B;
	float  Duration;

	DebugLinePacket(const FVector& InA, const FVector& InB, float InDuration = 10.0f)
		: Packet(EPacketType::S_DEBUG_LINE)
		, A(InA), B(InB), Duration(InDuration)
	{
		Header.PacketSize = sizeof(DebugLinePacket);
	}
};

#pragma endregion

#pragma region Friend
struct FriendAddRequestPacket : public Packet
{
	char TargetNickName[NICKNAME_LEN];

	FriendAddRequestPacket(const char* friendNick)
		: Packet(EPacketType::C_FRIEND_REQUEST)
	{
		SAFE_STRCPY(TargetNickName, friendNick, NICKNAME_LEN - 1);
		TargetNickName[NICKNAME_LEN - 1] = '\0';

		Header.PacketSize = sizeof(FriendAddRequestPacket);
	}
};

struct FriendRemoveRequestPacket : public Packet
{
	int32 TargetDBID;

	FriendRemoveRequestPacket(int32 targetDBId)
		: Packet(EPacketType::C_FRIEND_REMOVE), TargetDBID(targetDBId)
	{
		Header.PacketSize = sizeof(FriendRemoveRequestPacket);
	}
};

struct FriendOperationResultPacket : public Packet
{
	EFriendOpType OperationType;
	ResultCode ResCode;

	FriendOperationResultPacket(EFriendOpType type, ResultCode result)
		: Packet(EPacketType::S_FRIEND_OPERATION_RESULT), OperationType(type), ResCode(result)
	{
		Header.PacketSize = sizeof(FriendOperationResultPacket);
	}
};

struct FriendAcceptRequestPacket : public Packet
{
	int32 TargetDBID;

	FriendAcceptRequestPacket(int32 targetDBID)
		: Packet(EPacketType::C_FRIEND_ACCEPT), TargetDBID(targetDBID)
	{
		Header.PacketSize = sizeof(FriendAcceptRequestPacket);
	}
};

struct FriendRejectRequestPacket : public Packet
{
	int32 TargetDBID;

	FriendRejectRequestPacket(int32 targetDBID)
		: Packet(EPacketType::C_FRIEND_REJECT), TargetDBID(targetDBID)
	{
		Header.PacketSize = sizeof(FriendRejectRequestPacket);
	}
};

struct AddFriendPacket : public Packet
{
	FFriendInfo NewFriend;

	AddFriendPacket(const FFriendInfo& info)
		: Packet(EPacketType::S_ADD_FRIEND), NewFriend(info)
	{
		Header.PacketSize = sizeof(AddFriendPacket);
	}
};

struct RemoveFriendPacket : public Packet
{
	int32 FriendUserID;

	RemoveFriendPacket(int32 userId)
		: Packet(EPacketType::S_REMOVE_FRIEND), FriendUserID(userId)
	{
		Header.PacketSize = sizeof(RemoveFriendPacket);
	}
};

struct FriendRequestPacket : public Packet
{
	FFriendInfo RequesterInfo;

	FriendRequestPacket(const FFriendInfo& requesterInfo)
		: Packet(EPacketType::S_REQUEST_FRIEND), RequesterInfo(requesterInfo)
	{
		Header.PacketSize = sizeof(FriendRequestPacket);
	}
};

// Todo: 시간 되면 친구 로그아웃/로그인
#pragma endregion

#pragma pack(pop)