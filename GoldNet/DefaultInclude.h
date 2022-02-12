#ifndef _H_BASEINCLUDES
#define _H_BASEINCLUDES

#ifdef _MSC_VER
#pragma once
#endif

// Windows 
#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <debugapi.h>
#include <winternl.h>
#pragma comment(lib, "ws2_32.lib")
#include <Psapi.h>
#include <mswsock.h>

//#define DEBUGCON(_VA_ARGS_) printf(_VA_ARGS_)
//#define DEBUGCON __debugbreak(); printf
#define DEBUGCON printf
#elif __linux__ 

#endif

#include <intrin.h>
#include <array>
#include <cstring>
#include <string_view>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <array>
#include <memory>
#include <list>
#include <mutex>

#include "XOR_STR.h"

#endif // _H_BASEINCLUDES