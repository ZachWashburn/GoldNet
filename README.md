# GoldNet
Simple Drop-In Networking Library For Quickly Prototyping and Testing Code That Requires A Client->Server Setup

Simple library made for quickly testing windows Client->Server Applications. Wraps Winsocks2 API TCP non-blocking sockets. All imports are fetched dynamically at runtime,
allowing for no winsocks imports to appear in the import table of the PE Header. All specialized types that require an #Include are cast within the code,
allowing the GoldNet.h header to include no additionals libraries. Functions are thread safe.



AES256 Encryption and LZSS Compression Coming Soon!

Simple Use Is As Follows :

```
GN::NET_Initialize();
GN::NET_InitializeWinSocks();

Server:
	CTCPNetworkServer* pServer = new GN::TCP::CTCPNetworkServer();
	pServer->OpenPort("28901");
  
  while(true) { 
      pServer->Frame();
  };
     
Client:

  GN::IRawNetworkConnection* pConn = GN::TCP::NET_OpenTCPConnection("127.0.0.1", "28901");

```

Message Handling examples can be found in the GoldNetTests Project included with the source
