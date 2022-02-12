// GoldNet.cpp : Defines the functions for the static library.
//

#include "DefaultInclude.h"
#include "GoldNet.h"
#include "WindowsImports.h"
#include "NetHelpers.h"

// So As To Allow Keeping Types that require includes out of the header
#define CREATE_TYPE_CAST(type, variable) type* _##variable = reinterpret_cast<type*>(variable);


// https://www.codeproject.com/Articles/14462/Build-your-own-cryptographically-safe-server-clien

typedef void(__stdcall* OnClientAccept_t)(GN::IRawNetworkConnection* pConn);
SOCKET NET_IntrnlListenTCPSocket(const char* szPort);
GN::TCP::CTCPNetworkConnection* NET_AcceptTCPConnectionsNonBlocking(unsigned int sListen, int siTimeOut = 0, OnClientAccept_t fnCallback = nullptr);

struct mutex_wrapper
{
    std::mutex m_Lock;
};


struct OutgoingMessage_t
{
    OutgoingMessage_t(GN::CRawNetworkMessage* msg, size_t nBytes = 0) { pMsg = msg; }
    ~OutgoingMessage_t() {
        pMsg->FreeMsg(); 
    };
    GN::CRawNetworkMessage* pMsg;
    size_t m_nBytesSent = 0;
};

struct GoldNetGlobals
{
    std::mutex m_ConnectionLock;
    std::list<GN::IRawNetworkConnection*> m_Connections;
    std::list<GN::IRawNetworkConnection*> m_Clients;
};
GoldNetGlobals g_Globals;
GoldNetGlobals* g_pGlobals = &g_Globals; // Maybe alloc this at runtime?


GN::IRawNetworkConnection::IRawNetworkConnection()
{
    m_pMessagesList = new std::list<GN::CRawNetworkMessage*>();
    m_pMessagesLock = new mutex_wrapper();

    m_pOutgoingMessagesList = new std::list<OutgoingMessage_t*>();
    m_pOutgoingMessagesLock = new mutex_wrapper();

    m_Socket = INVALID_SOCKET;
}

GN::IRawNetworkConnection::~IRawNetworkConnection()
{
    _shutdown(m_Socket, SD_BOTH);
    _closesocket(m_Socket);
    delete (std::list<GN::CRawNetworkMessage*>*)m_pMessagesList;
    delete (mutex_wrapper*)m_pMessagesLock;
    delete (std::list<OutgoingMessage_t*>*)m_pOutgoingMessagesList;
    delete (mutex_wrapper*)m_pOutgoingMessagesLock;
}
void GN::IRawNetworkConnection::SendMsg(CRawNetworkMessage* pMsg)
{
    CREATE_TYPE_CAST(mutex_wrapper, m_pOutgoingMessagesLock);
    CREATE_TYPE_CAST(std::list<OutgoingMessage_t*>, m_pOutgoingMessagesList);

    _m_pOutgoingMessagesLock->m_Lock.lock();
    _m_pOutgoingMessagesList->push_front(new OutgoingMessage_t(pMsg, 0));
    _m_pOutgoingMessagesLock->m_Lock.unlock();

    Update();
}

_Ret_maybenull_ GN::CRawNetworkMessage** GN::IRawNetworkConnection::GetMessages(_In_ int nMaxCount, _In_ int& nCountOut)
{
    CREATE_TYPE_CAST(std::list<GN::CRawNetworkMessage*>, m_pMessagesList)
    std::lock_guard<std::mutex> _(((mutex_wrapper*)m_pMessagesLock)->m_Lock);

    nCountOut = min(nMaxCount, _m_pMessagesList->size());

    if (!nCountOut)
        return nullptr;

    CRawNetworkMessage** pRet = (CRawNetworkMessage**)malloc(sizeof(CRawNetworkMessage*) * nCountOut);

    if (!pRet)
        return nullptr;

    for (int i = 0; i < nCountOut; i++)
    {
        pRet[i] = _m_pMessagesList->back();
        _m_pMessagesList->pop_back();
    }

    return pRet;
}


void GN::TCP::CTCPNetworkConnection::UpdateIncoming()
{
    if (!m_pDataBuffer)
    {
        m_nCurrentBufferSize = sizeof(MessageHeader_t);
        m_nCurrentBytesRecieved = 0;
        m_pDataBuffer = malloc(m_nCurrentBufferSize);
        m_bCurrentRecievingMessageData = false;

        if (!m_pDataBuffer)
            return;

    }

    int iResult = _recv(m_Socket, (char*)m_pDataBuffer + m_nCurrentBytesRecieved, m_nCurrentBufferSize - m_nCurrentBytesRecieved, NULL);

    if (iResult == SOCKET_ERROR)
    {
        iResult = _WSAGetLastError();
        switch (iResult)
        {

        case WSAEWOULDBLOCK:
            break;
        case WSAENOTCONN:
        case WSAENOTSOCK:
        case WSAESHUTDOWN:
        case WSAECONNABORTED:
        case WSAETIMEDOUT:
        case WSAECONNRESET:
        default:
            m_bMarkedForDeletion = true;
            return;
        }

        return;
    }
    else if (iResult == 0)
    {
        m_bMarkedForDeletion = true;
        return;
    }
    else
    {
        m_nCurrentBytesRecieved += iResult;
    }

    auto FailRecieve = [&]() {
        m_bCurrentRecievingMessageData = false;
        if(m_pDataBuffer)
            free(m_pDataBuffer);
        m_pDataBuffer = nullptr;
        m_nCurrentBufferSize = 0;
    };

    if (!m_bCurrentRecievingMessageData)
    {
        if (m_nCurrentBytesRecieved == sizeof(MessageHeader_t))
        {
            m_PreviousMessageHeader = *(MessageHeader_t*)m_pDataBuffer;
            m_bCurrentRecievingMessageData = true;

            if (!strstr(m_PreviousMessageHeader.Iden, IDENTIFIER) || !m_PreviousMessageHeader.m_nMessageSize)
            {
                FailRecieve();
                return;
            }
            m_nCurrentBufferSize = m_PreviousMessageHeader.m_nMessageSize;
            m_pDataBuffer = malloc(m_nCurrentBufferSize);
            m_nCurrentBytesRecieved = 0;
            m_bCurrentRecievingMessageData = true;
        }
    }
    else
    {
        if (m_nCurrentBufferSize != m_nCurrentBytesRecieved)
            return;

        std::lock_guard<std::mutex> _(((mutex_wrapper*)m_pMessagesLock)->m_Lock);
        std::list<GN::CRawNetworkMessage*>* pMessages = (std::list<GN::CRawNetworkMessage*>*)m_pMessagesList;

        // Decompress

        // Decrypt



        CRawNetworkMessage* pMsg = new CRawNetworkMessage(m_pDataBuffer, m_nCurrentBufferSize, m_PreviousMessageHeader.m_nMessageType);
        pMessages->push_back(pMsg);
        FailRecieve(); // Just call it that, we actually suceeded!
    }
}


void GN::TCP::CTCPNetworkConnection::UpdateOutgoing()
{
    std::lock_guard<std::mutex> _(((mutex_wrapper*)m_pOutgoingMessagesLock)->m_Lock);
    auto pOutgoing = ((std::list<OutgoingMessage_t*>*)m_pOutgoingMessagesList);
    while (!pOutgoing->empty())
    {
        auto pMsg = pOutgoing->back();
        int iResult = _send(m_Socket, (const char*)pMsg->pMsg->GetFullData(), pMsg->pMsg->GetFullDataSize() - pMsg->m_nBytesSent, NULL);

        if (iResult == SOCKET_ERROR)
        {
            iResult = _WSAGetLastError();
            switch (iResult)
            {
            case WSAENOTCONN:
            case WSAENOTSOCK:
            case WSAESHUTDOWN:
            case WSAECONNABORTED:
            case WSAETIMEDOUT:
            case WSAECONNRESET:
                m_bMarkedForDeletion = true;
                return;
            case WSAEWOULDBLOCK:
                break;
            default:
                m_bMarkedForDeletion = true;
                break;
            }

            return;
        }

        pMsg->m_nBytesSent += iResult;
        if (pMsg->m_nBytesSent == pMsg->pMsg->GetFullDataSize())
        {
            delete pOutgoing->back();
            pOutgoing->pop_back();
        }
    }
}

void GN::TCP::CTCPNetworkConnection::Update()
{
    if (m_bMarkedForDeletion)
        return;

    UpdateIncoming();
    UpdateOutgoing();

}




GN::TCP::CTCPNetworkServer::CTCPNetworkServer()
{
    m_pClients = new std::list<CTCPNetworkConnection*>();
    m_pClientLock = new std::mutex();
    m_pDeleteLock = new std::mutex();
    m_pDeletionListLock = new std::mutex();
    m_pDeletables = new std::vector<CTCPNetworkConnection*>();
}

GN::TCP::CTCPNetworkServer::CTCPNetworkServer(const char* szPort)
{
    m_pClients = new std::list<CTCPNetworkConnection*>();
    m_pClientLock = new std::mutex();
    m_pDeleteLock = new std::mutex();
    m_pDeletionListLock = new std::mutex();
    m_pDeletables = new std::vector<CTCPNetworkConnection*>();

    m_ListeningSocket = NET_IntrnlListenTCPSocket(szPort);
}

bool GN::TCP::CTCPNetworkServer::OpenPort(const char* szPort)
{
    m_ListeningSocket = NET_IntrnlListenTCPSocket(szPort);

    if (m_ListeningSocket == INVALID_SOCKET)
        return false;

    return true;
}

GN::TCP::CTCPNetworkServer::~CTCPNetworkServer()
{
    delete (std::mutex*)m_pClientLock;
    delete (std::mutex*)m_pDeleteLock;
    delete (std::mutex*)m_pDeletionListLock;
    delete (std::list<CTCPNetworkConnection*>*)m_pClients;
    delete (std::vector<CTCPNetworkConnection*>*)m_pDeletables;

}

void GN::TCP::CTCPNetworkServer::ClientFrame()
{
    // Update and manage Current Connections
    std::mutex* pClientLock = (std::mutex*)m_pClientLock;
    std::list<CTCPNetworkConnection*>* m_Clients = reinterpret_cast<std::list<CTCPNetworkConnection*>*>(m_pClients);
    std::list<CTCPNetworkConnection*> Clients;
    std::mutex* pDeleteLock = (std::mutex*)m_pDeleteLock;
    std::vector<CTCPNetworkConnection*> deletables;


    // Lock Client Access
    pClientLock->lock();
    
    // Copy Over Client Data
    Clients = *m_Clients;

    // Release the client list to be modified
    pClientLock->unlock();

    // Lock Deletions
    LockClients();

    // Handle Connections and updates etc
    for (const auto& cli : Clients)
    {
        if (cli->bIsMarkedForDeletion())
            deletables.push_back(cli);
        else
            cli->Update();
    }

    // Unlock Clients
    UnlockClients();


    // Copy over any clients we want to delete 
    if (pClientLock->try_lock())
    {
        // Lock our Deletion List
        std::lock_guard<std::mutex>(*(std::mutex*)m_pDeletionListLock);
        for (const auto& cli : deletables)
        {
            ((std::vector<CTCPNetworkConnection*>*)m_pDeletables)->push_back(cli);
            m_Clients->erase(std::find(m_Clients->begin(), m_Clients->end(), cli));
        }

        pClientLock->unlock();
    }

}

void GN::TCP::CTCPNetworkServer::ConnectionsFrame()
{
    std::mutex* pClientLock = (std::mutex*)m_pClientLock;
    std::mutex* pDeleteLock = (std::mutex*)m_pDeleteLock;
    std::mutex* pDeletionListLock = (std::mutex*)m_pDeletionListLock;
    std::list<CTCPNetworkConnection*>* m_Clients = reinterpret_cast<std::list<CTCPNetworkConnection*>*>(m_pClients);
    std::vector<CTCPNetworkConnection*> deletables;
    static std::list< GN::TCP::CTCPNetworkConnection*> conn_list;

    // Lock and remove any deletions
    if (pDeletionListLock->try_lock())
    {
        deletables = *((std::vector<CTCPNetworkConnection*>*)m_pDeletables);
        ((std::vector<CTCPNetworkConnection*>*)m_pDeletables)->clear();
        pDeletionListLock->unlock();

        pDeleteLock->lock();
        for (const auto& cli : deletables)
            delete cli;
        
        pDeleteLock->unlock();
    }

    // Accept New Connections
    int i = 0;
    GN::TCP::CTCPNetworkConnection* pConn;
    do {
        pConn = NET_AcceptTCPConnectionsNonBlocking(m_ListeningSocket);
        if (!pConn)
            break;
       
        conn_list.push_front(pConn);
        i++;
    } while (pConn && (i < 20));

    // Add New Connections to list
    if (pClientLock->try_lock())
    {
        m_Clients->merge(conn_list);
        conn_list.clear();
        pClientLock->unlock();
    }
}

void GN::TCP::CTCPNetworkServer::LockClients()
{
    ((std::mutex*)m_pDeleteLock)->lock();
}

void GN::TCP::CTCPNetworkServer::UnlockClients()
{
    ((std::mutex*)m_pDeleteLock)->unlock();
}

GN::TCP::CTCPNetworkConnection** GN::TCP::CTCPNetworkServer::GetClients(unsigned int& nClientReturned)
{
    //std::lock(*(std::mutex*)m_pClientLock, *(std::mutex*)m_pDeleteLock);
    //std::lock_guard<std::mutex> _1(*(std::mutex*)m_pClientLock, std::adopt_lock);
    //std::lock_guard<std::mutex> _2(*(std::mutex*)m_pDeleteLock, std::adopt_lock);


    std::lock_guard<std::mutex> _(*(std::mutex*)m_pClientLock);
    std::list<CTCPNetworkConnection*>* m_Clients = reinterpret_cast<std::list<CTCPNetworkConnection*>*>(m_pClients);

    nClientReturned = m_Clients->size();

    if (!nClientReturned)
        return nullptr;

    CTCPNetworkConnection** arrClients = (CTCPNetworkConnection * *)malloc(sizeof(CTCPNetworkConnection*) * m_Clients->size());

    if (!arrClients)
        return nullptr;

    std::copy(m_Clients->begin(), m_Clients->end(), arrClients);

    return arrClients;
}

void GN::TCP::CTCPNetworkServer::FreeClients(CTCPNetworkConnection** pArray)
{
    if(pArray)
        free(pArray);
}




int GN::NET_Initialize()
{
	WindowsImportsInitialize();
    return 0;
}

int GN::NET_InitializeWinSocks()
{
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = _WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        DEBUGCON(XorStr("WSAStartup failed: %d\n"), iResult);
        return 1;
    }

    return 0;
}


// Nonblocking Connect
unsigned int NET_IntrnlConnectTCP(_In_z_ const char* szAddr, _In_z_ const char* szPort)
{
    SOCKET sConn = INVALID_SOCKET;
    SOCKET sStateCheck = INVALID_SOCKET;
    INT iResult;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;

    static auto fnCheckSocketState = [](SOCKET sSocket) -> SOCKET
    {
        HRESULT iResult = _WSAGetLastError();

        switch (iResult)
        {
        case 0:
        case WSAEISCONN:
            return sSocket;
            break;
        case WSAEWOULDBLOCK:
        case WSAEALREADY:
            break; // we need to wait a tad bit longer
        case WSAEINVAL:
            DEBUGCON(XorStr("Invalid Argument Supplied To connect\n"));
            return SOCKET_ERROR;
        default:
            DEBUGCON(XorStr("Client connect() error %d\n"), iResult);
            return SOCKET_ERROR;
        }

        return 0;
    };

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = _getaddrinfo(szAddr, szPort, &hints, &result);
    if (iResult != 0) {
        DEBUGCON(XorStr("getaddrinfo failed with error: %d\n"), iResult);
        _WSACleanup();
        return INVALID_SOCKET;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        sConn = _socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (sConn == INVALID_SOCKET) {
            DEBUGCON(XorStr("socket failed with error: %ld\n"), _WSAGetLastError());
            //_freeaddrinfo(result);
            _WSACleanup();
            return INVALID_SOCKET;
        }

        if (!NET_SetSocketBlocking(sConn, false))
        {
            DEBUGCON(XorStr("socket failed to enter blocking mode: %ld\n"), _WSAGetLastError());
            //_freeaddrinfo(result);
            _WSACleanup();
            return INVALID_SOCKET;
        }

        // Connect to server.
        iResult = _connect(sConn, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            //_freeaddrinfo(result);
            //_closesocket(sConn);
            //sConn = INVALID_SOCKET;
            continue;
        }

        break;
    }

    if(sConn != INVALID_SOCKET)
        _freeaddrinfo(result);

    sStateCheck = fnCheckSocketState(sConn);

    if (sStateCheck != 0) // Logic Flaw Maybe?
        return sStateCheck && sStateCheck != SOCKET_ERROR;

    // Technically this is blocking, maybe a better solution is required?
    // Wait until SOCKET_ERROR or Connection Succeeds 
    while (NET_GetSocketStatus(sConn, STATUS_WRITE) != 0)
    {
        sStateCheck = fnCheckSocketState(sConn);

        if (sStateCheck != 0 && sStateCheck != SOCKET_ERROR)
            return sStateCheck;
    }
}

_Ret_maybenull_ GN::IRawNetworkConnection* GN::TCP::NET_OpenTCPConnection(_In_z_ const char* szAddr, _In_z_ const char* szPort)
{
    SOCKET sSocket = NET_IntrnlConnectTCP(szAddr, szPort);

    if (sSocket == SOCKET_ERROR)
        return nullptr;

    CTCPNetworkConnection* ConnObj = new CTCPNetworkConnection(sSocket);

    g_pGlobals->m_ConnectionLock.lock();
    g_pGlobals->m_Connections.push_front(ConnObj);
    g_pGlobals->m_ConnectionLock.unlock();
   
    return ConnObj;
}

void GN::TCP::NET_OpenTCPConnectionAsync(
    _In_z_ const char* szAddr,
    _In_z_ const char* szPort,
    _In_ GN::ConnectionCreationCallbackFunc_t fnCallback
)
{
    GN::IRawNetworkConnection* pConn = NET_OpenTCPConnection(szAddr, szPort);
    fnCallback(pConn);
}


SOCKET NET_IntrnlListenTCPSocket(const char* szPort)
{
    INT iResult;
    SOCKET sListen = INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = _getaddrinfo(NULL, szPort, &hints, &result);
    if (iResult != 0) {
        DEBUGCON(XorStr("getaddrinfo failed with error: %d\n"), iResult);
        _WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    sListen = _socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sListen == INVALID_SOCKET) {
        DEBUGCON(XorStr("socket failed with error: %ld\n"), _WSAGetLastError());
        _freeaddrinfo(result);
        _WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = _bind(sListen, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        DEBUGCON(XorStr("bind failed with error: %d\n"), _WSAGetLastError());
        _freeaddrinfo(result);
        _closesocket(sListen);
        _WSACleanup();
        return 1;
    }

    _freeaddrinfo(result);

    iResult = _listen(sListen, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        DEBUGCON(XorStr("listen failed with error: %d\n"), _WSAGetLastError());
        _closesocket(sListen);
        _WSACleanup();
        return 1;
    }
    if (!NET_SetSocketBlocking(sListen, false))
    {
        DEBUGCON(XorStr("socket failed to enter blocking mode: %ld\n"), _WSAGetLastError());
        _WSACleanup();
        return INVALID_SOCKET;
    }

    return sListen;
}


unsigned int ListenSocket(const char* szPort)
{
    return NET_IntrnlListenTCPSocket(szPort);
}

typedef void(__stdcall* OnClientAccept_t)(GN::IRawNetworkConnection* pConn);
GN::TCP::CTCPNetworkConnection* NET_AcceptTCPConnectionsNonBlocking(unsigned int sListen, int siTimeOut/*= 0*/, OnClientAccept_t fnCallback /*= nullptr*/)
{
    WSAPOLLFD fdArray[1];
    INT iResult;
    SOCKADDR_IN  pAddr;
    int addr_len = sizeof(SOCKADDR_IN);
    char szAddr[4096];

    fdArray[0].fd = sListen;
    fdArray[0].events &= POLLRDNORM;

    iResult = _WSAPoll(fdArray, 1, siTimeOut);

    if (iResult < 0)
        return nullptr;

    SOCKET sClient = _accept(sListen, (struct sockaddr*)&pAddr, &addr_len);

    if (sClient == SOCKET_ERROR)
        return nullptr;

    _inet_ntop(pAddr.sin_family, &pAddr.sin_addr, szAddr, ARRAYSIZE(szAddr));
    GN::TCP::CTCPNetworkConnection* pConn = new GN::TCP::CTCPNetworkConnection(sClient, szAddr, pAddr.sin_port);

    g_pGlobals->m_ConnectionLock.lock();
    g_pGlobals->m_Clients.push_front(pConn);
    g_pGlobals->m_ConnectionLock.unlock();

    if (fnCallback)
        fnCallback(pConn);

    return pConn;
}

bool GN::TCP::NET_OpenTCPListeningSocket
(
    _In_z_ const char* szPort
)
{
    return NET_IntrnlListenTCPSocket(szPort) ? true : false;
}