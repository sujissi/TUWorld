#pragma once

#define SAFE_STRCPY(dest, src, size) strncpy_s(dest, size + 1, src, size)

#ifdef SERVER_BUILD
using BYTE = unsigned char;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
#else
// 클라이언트에서 서버 접속 시 사용
constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int         SERVER_PORT = 4000;
#endif

constexpr uint32 BUFSIZE = 1024;
constexpr int LOGIN_STR_LEN = 20 + 1;
constexpr int NICKNAME_LEN = 30 + 1;
constexpr int CHAT_MESSAGE_LEN = 100 + 1;
constexpr int WORLD_MAX_COUNT = 8;
constexpr uint32 ROCK_DELAY_MS = 500;

#include "Type.h"
#include "DataStruct.h"
#include "Packet.h"