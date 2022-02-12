#pragma once
#include "DefaultInclude.h"

enum SocketStatus
{
	STATUS_READ = 0x1,
	STATUS_WRITE = 0x2, 
	STATUS_EXCEPT = 0x4
};

int NET_GetSocketStatus(const SOCKET a_socket, int status);
bool NET_SetSocketBlocking(int fd, bool blocking);