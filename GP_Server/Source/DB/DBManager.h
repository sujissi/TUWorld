#pragma once

struct DBLoginResult
{
	ResultCode code;
	uint32 dbId = 0;
	FInfoData info;
	std::vector<std::pair<uint32, uint8>> items;
	std::vector<FFriendInfo> friends;
};


class DBManager
{
public:
	static DBManager& GetInst() {
		static DBManager inst;
		return inst;
	}
	bool Connect(const std::string& host, const std::string& user, const std::string& pass, const std::string& schema, int32 poolSize);
	void Shutdown();
	DBLoginResult SignUpUser(int32 sessionId, const std::string& login_id, const std::string& password, const std::wstring& nickname);
	DBLoginResult CheckLogin(int32 sessionId, const std::string& login_id, const std::string& password);
	bool UpdatePlayerInfo(uint32 dbId, const FInfoData& info);
	bool AddUserItem(uint32 dbId, uint8 itemTypeID);
	bool RemoveUserItem(uint32 dbId, uint32 itemID);
	bool DoesUserExist(uint32 userId);

	ResultCode AddFriendRequest(uint32 fromId, uint32 toId);
	bool IsFriendOrPending(uint32 userId, uint32 targetId);
	std::pair<ResultCode, std::optional<FFriendInfo>> AcceptFriendRequest(uint32 fromId, uint32 toId);
	ResultCode RejectFriendRequest(uint32 fromId, uint32 toId);
	ResultCode RemoveFriend(uint32 userId, uint32 friendId);
	std::pair<ResultCode, std::vector<FFriendInfo>> GetFriendList(uint32 userId);
	int32 FindUserDBId(const std::wstring& nickname);

};