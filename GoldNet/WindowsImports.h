#ifndef _H_WINDOWSIMPORTS
#define _H_WINDOWSIMPORTS
#ifdef _WIN32

#ifdef _MSC_VER
#pragma once
#endif // _MSC_VER
#include "DefaultInclude.h"

/*
	Hide Function Imports from the import table by loading them dynamically.
	Simple Add The Def here and loader in WindowsImports.cpp, since sometimes
	we don't want people scanning our imports!

	all function names remain the same except we add a "_" prefixing the function
*/


#define ImportedFunctionDef(FuncName) inline decltype(&FuncName) _##FuncName = nullptr;
#define FetchDLL(DLLName, DLLHandle) DLLHandle = _GetModuleHandleA(XorStr(DLLName)); if(!DLLHandle){DLLHandle = _LoadLibraryA(XorStr(DLLName));};
#ifdef _DEBUG
#define FetchFunction(FuncName, DLL) _##FuncName = reinterpret_cast<decltype(&FuncName)>(_GetProcAddress(DLL, XorStr(#FuncName))); if(!_##FuncName){__debugbreak();}
#else
#define FetchFunction(FuncName, DLL) _##FuncName = reinterpret_cast<decltype(&FuncName)>(_GetProcAddress(DLL, XorStr(#FuncName)));

#endif

// Globals 
inline HMODULE g_hWinSockets = NULL;
inline HMODULE g_hKernel32 = NULL;

// Kernel32.dll
ImportedFunctionDef(LoadLibraryA);
ImportedFunctionDef(LoadLibraryW);
ImportedFunctionDef(GetProcAddress);
ImportedFunctionDef(GetModuleHandleA);



// WsaSocks
ImportedFunctionDef(WSAStartup);
ImportedFunctionDef(WSACleanup);
ImportedFunctionDef(getaddrinfo);
ImportedFunctionDef(socket);
ImportedFunctionDef(freeaddrinfo);
ImportedFunctionDef(WSAGetLastError);
ImportedFunctionDef(shutdown);
ImportedFunctionDef(ioctlsocket);
ImportedFunctionDef(connect);
ImportedFunctionDef(closesocket);
ImportedFunctionDef(getsockopt);
ImportedFunctionDef(setsockopt);
ImportedFunctionDef(__WSAFDIsSet);
ImportedFunctionDef(WSAAsyncSelect);
ImportedFunctionDef(select);
ImportedFunctionDef(bind);
ImportedFunctionDef(listen);
ImportedFunctionDef(accept);
ImportedFunctionDef(WSAPoll);
ImportedFunctionDef(inet_ntop);
ImportedFunctionDef(recv);
ImportedFunctionDef(send);

void WindowsImportsInitialize();


#endif // _WIN32
#endif // _H_WINDOWSIMPORTS