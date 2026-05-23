#pragma once
#define NOMINMAX

// SERVER_BUILD가 정의되면 Common.h에서 SERVER_IP/PORT가 빠지므로 여기서 보충
constexpr const char* SERVER_IP   = "127.0.0.1";
constexpr int         SERVER_PORT = 4000;
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <memory>
#include <array>
#include <queue>
#include <unordered_set>
#include <functional>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <format>

#include "IOCP.h"
#include "FVector.h"
#include "Common.h"
#include "ConfigManager.h"
#define LOG_I(fmt, ...) ((void)0)
#define LOG_W(fmt, ...) ((void)0)
#define LOG_E(fmt, ...) ((void)0)
#define LOG_D(fmt, ...) ((void)0)
#include "RandomUtils.h"
#include "Map.h"
#include "ExpOver.h"
#include "TimerQueue.h"

using namespace std::chrono;

inline long long NowMs()
{
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

inline int delayTime;
constexpr int DELAY_LIMIT = 100;
constexpr int DELAY_LIMIT2 = 300;
constexpr int ACCEPT_DELY = 40;

inline void UpdateDelay(int rtt_ms) {
	if (delayTime < rtt_ms) delayTime++;
	else if (delayTime > rtt_ms) delayTime--;
}

inline std::atomic_int _active_clients;
inline constexpr size_t MAX_CLIENT = 2000;
inline constexpr size_t MAX_PLAYER = MAX_CLIENT;
inline constexpr float VIEW_DIST = 5000.f;
inline constexpr float playerCollision = 50.f;

const std::string BasePath = "../GP_Server";

const std::string MapDataPath = BasePath + "/Data/MapJsonData/";
const std::string DataTablePath = BasePath + "/Data/DataTable/";

enum class CompType
{
	RECV,
	SEND,
	ACCEPT,
};

static std::wstring ConvertToWString(const std::string& str)
{
	int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (size <= 0) return L"";

	std::wstring wstr(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);

	wstr.pop_back();
	return wstr;
}

// ─────────────────────────────────────────────────────────
// 1초 단위 부하 테스트 통계
// ─────────────────────────────────────────────────────────
struct PerSecStats {
	std::atomic<long long> moveSent{ 0 };
	std::atomic<long long> rttSum{ 0 };
	std::atomic<int>       rttCount{ 0 };
	std::atomic<long long> rttMin{ -1 };
	std::atomic<long long> rttMax{ -1 };

	void RecordRtt(long long rtt) {
		if (rtt < 0 || rtt > 60000) return;
		rttSum.fetch_add(rtt);
		rttCount.fetch_add(1);
		long long cur;
		cur = rttMin.load(std::memory_order_relaxed);
		while ((cur < 0 || rtt < cur) && !rttMin.compare_exchange_weak(cur, rtt, std::memory_order_relaxed));
		cur = rttMax.load(std::memory_order_relaxed);
		while (rtt > cur && !rttMax.compare_exchange_weak(cur, rtt, std::memory_order_relaxed));
	}

	void Reset() {
		moveSent = 0; rttSum = 0; rttCount = 0;
		rttMin = -1; rttMax = -1;
	}
};

inline PerSecStats g_stats;