#pragma once
#ifndef _DEBUG
#define DB_MODE
#endif

#define TEST 1

//for test
constexpr int TEST_ATK_WEIGHT = 5;
constexpr int TEST_EXP_WEIGHT = 10;

constexpr int GOOD_STATE_WORLD = 200;
constexpr int NORMAL_STATE_WORLD = 400;

constexpr size_t MAX_CLIENT = 10000;
constexpr size_t MAX_PLAYER = MAX_CLIENT;
constexpr size_t MAX_MONSTER = 500;
constexpr size_t MAX_CHARACTER = MAX_PLAYER + MAX_MONSTER;

constexpr float VIEW_DIST = 4000.f;
constexpr float GRID_CELL_SIZE = 2000.f;

constexpr float playerCollision = 50.f;
constexpr float DfAtkRadius = 200;
constexpr float DFfovAngle = 120;
constexpr float DfWarriorAtkRadius = 300;
constexpr float DFWarriorfovAngle = 90;
constexpr float DfGunnerAtkRadius = 5000;
constexpr float DFGunnerfovAngle = 10;

constexpr int ITEM_DISAPPEAR_TIME_MS = 60 * 1000;
constexpr int MONSTER_RESPAWN_TIME_MS = 10 * 1000;

const std::string BasePath = std::filesystem::current_path().string();

const std::string MapDataPath = BasePath + "/MapJsonData/";
const std::string DataTablePath = BasePath + "/DataTable/";

#define ENUM_NAME(x) (magic_enum::enum_name(x).empty() ? "UnknownEnum" : std::string(magic_enum::enum_name(x)))

enum class CompType
{
	RECV,
	SEND,
	ACCEPT,
};

inline std::wstring ConvertToWString(const std::string& str)
{
	int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (size <= 0) return L"";

	std::wstring wstr(size - 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
	return wstr;
}
