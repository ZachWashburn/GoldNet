#ifdef _WIN32
#include "WindowsImports.h"


void WindowsImportsInitialize()
{
	g_hKernel32 = GetModuleHandleA(XorStr("kernel32.dll"));

	_GetModuleHandleA = (decltype(&GetModuleHandleA))GetProcAddress(g_hKernel32, XorStr("GetModuleHandleA"));
	_GetProcAddress = (decltype(&GetProcAddress))GetProcAddress(g_hKernel32, XorStr("GetProcAddress"));
	FetchFunction(LoadLibraryA, g_hKernel32);
	FetchFunction(LoadLibraryW, g_hKernel32);


	// Get DLLs
	FetchDLL("Ws2_32.dll", g_hWinSockets);
	


	// Winsocks2 

	FetchFunction(WSAStartup, g_hWinSockets);
	FetchFunction(WSACleanup, g_hWinSockets);
	FetchFunction(getaddrinfo, g_hWinSockets);
	FetchFunction(socket, g_hWinSockets);
	FetchFunction(freeaddrinfo, g_hWinSockets);
	FetchFunction(WSAGetLastError, g_hWinSockets);
	FetchFunction(shutdown, g_hWinSockets);
	FetchFunction(ioctlsocket, g_hWinSockets);
	FetchFunction(connect, g_hWinSockets);
	FetchFunction(closesocket, g_hWinSockets);
	FetchFunction(getsockopt, g_hWinSockets);
	FetchFunction(setsockopt, g_hWinSockets);
	FetchFunction(__WSAFDIsSet, g_hWinSockets);
	FetchFunction(WSAAsyncSelect, g_hWinSockets);
	FetchFunction(select, g_hWinSockets);
	FetchFunction(bind, g_hWinSockets);
	FetchFunction(listen, g_hWinSockets);
	FetchFunction(accept, g_hWinSockets);
	FetchFunction(WSAPoll, g_hWinSockets);
	FetchFunction(inet_ntop, g_hWinSockets);
	FetchFunction(recv, g_hWinSockets);
	FetchFunction(send, g_hWinSockets);
}



#endif