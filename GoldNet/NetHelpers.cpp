#include "NetHelpers.h"
#include "WindowsImports.h"

// https://pastebin.com/JkGnQyPX
int NET_GetSocketStatus(const SOCKET a_socket, int status)
{
    // zero seconds, zero milliseconds. max time select call allowd to block
    static timeval instantSpeedPlease = { 0,0 };
    fd_set a = { 1, {a_socket} };
    fd_set* read = ((status & 0x1) != 0) ? &a : NULL;
    fd_set* write = ((status & 0x2) != 0) ? &a : NULL;
    fd_set* except = ((status & 0x4) != 0) ? &a : NULL;
    /*
    select returns the number of ready socket handles in the fd_set structure, 
    zero if the time limit expired, or SOCKET_ERROR if an error occurred.
    WSAGetLastError can be used to retrieve a specific error code.
    */
    int result = _select(0, read, write, except, &instantSpeedPlease);
    if (result == SOCKET_ERROR)
    {
        result = _WSAGetLastError();
    }
    if (result < 0 || result > 3)
    {
        DEBUGCON(XorStr("select(read) error %d\n"), result);
        return SOCKET_ERROR;
    }
    return result;
}

/** Returns true on success, or false if there was an error */
// https://stackoverflow.com/questions/1543466/how-do-i-change-a-tcp-socket-to-be-non-blocking/1549344
bool NET_SetSocketBlocking(int fd, bool blocking)
{
    if (fd < 0) return false;
    unsigned long mode = blocking ? 0 : 1;
    return (_ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
}