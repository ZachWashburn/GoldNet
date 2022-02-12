// GoldNetTests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "../GoldNet/GoldNet.h"
#include <thread>

#include <Windows.h>
#include <string>
// TODO : Lock Deletion!

GN::TCP::CTCPNetworkServer* g_pServer = nullptr;;
void HandleAccepts()
{
	while (true)
	{
		g_pServer->ConnectionsFrame();
	}
}

void HandleClientData()
{
	while (true)
	{
		g_pServer->ClientFrame();
	}
}

void HandleMessages()
{
	while (true)
	{
		unsigned int nClients = 0;
		GN::TCP::CTCPNetworkConnection* pConn = nullptr;
		g_pServer->LockClients();
		auto Clients = g_pServer->GetClients(nClients);
		for (int i = 0; i < nClients; i++)
		{
			pConn = Clients[i];

			if (pConn->bIsMarkedForDeletion())
				continue;

			int nMessagesReturned = 0;
			GN::CRawNetworkMessage** Messages = pConn->GetMessages(INT_MAX, nMessagesReturned);
			GN::CRawNetworkMessage* pMsg = 0;
			for (int j = 0; j < nMessagesReturned; j++)
			{
				pMsg = Messages[j];
				printf((const char*)pMsg->GetRawData());
				pMsg->FreeMsg();
			}


		
		}

		g_pServer->FreeClients(Clients);
		g_pServer->UnlockClients();

	}
}

int main()
{
	GN::NET_Initialize();
	GN::NET_InitializeWinSocks();
	g_pServer = new GN::TCP::CTCPNetworkServer();
	g_pServer->OpenPort("28901");
	Sleep(400);
	std::thread newThread(&HandleAccepts);
	std::thread newThread2(&HandleClientData);
	std::thread newThread3(&HandleMessages);
	Sleep(1000);

	auto ret = GN::TCP::NET_OpenTCPConnection("127.0.0.1", "28901");
	int i = 0;
	while(true){
		std::string string = std::string(std::to_string(i) + std::string("\n\0"));
		ret->SendMsg(new GN::CRawNetworkMessage((void*)string.data(), string.size(), 2));
		//delete ret;
		Sleep(1);
		//ret = GN::TCP::NET_OpenTCPConnection("127.0.0.1", "28901");
		//ret->Update();
		i++;
	}
}



