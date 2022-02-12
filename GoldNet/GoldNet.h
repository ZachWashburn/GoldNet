#pragma once

// I'm Not A Particular Fan of Namespaces Used To Heavily
// How I Feel In This case, it provides a good organizational
// method for keeping everything clean!
// All sockets are non-blocking, keep this in mind!
// All networking Imports are handled at runtime by default, to hide them from
// being in the import tables

// This library provides a simple easy method for transmitting data though the CNetworkMessage
// class type. Basically a Simple Plug-in-Play networking library. This isn't for super important
// implementations, just whereever you need a easy simple way to transmit data.
// No includes are included in this header, which makes the source a little messier. I feel like
// Not requiring more library includes (other than sal) is the correct way to go here.

// The Setup here is a bit more convoluted than it could be, I could simplify everything down
// and have message callbacks, a more robust server, etc. But I want the flexibility, and with
// that comes a bit more setup on the user end.

//#include <sal.h>

#define IDENTIFIER "X10AS\0"

namespace GN
{

	enum GoldNetConfig {
		k_GoldNetCfg_Encryption,
		k_GoldNetCfg_Compression,
		_k_GoldNetCfg_CONFIG_MAX,
	};

#pragma pack(push, 1)

	struct MessageInfo_t
	{
		bool m_bIsCompressed = false;
		bool m_bIsEncrypted = false;
	};

	struct MessageHeader_t
	{
		char Iden[sizeof(IDENTIFIER)] = IDENTIFIER;
		uint32_t m_nMessageSize = 0;
		uint32_t m_nMessageType = 0;
	};
#pragma pack(pop)

	class CRawNetworkMessage;
	class IRawNetworkConnection;
	typedef void(__stdcall* ConnectionCreationCallbackFunc_t)(IRawNetworkConnection*);

	class IRawNetworkConnection
	{
	public:
		IRawNetworkConnection();
		~IRawNetworkConnection();
		virtual void Update() = 0;
		_Ret_maybenull_ CRawNetworkMessage** GetMessages(_In_ int nMaxCount, _In_ int& nCountOut);
		void SendMsg(CRawNetworkMessage* pMsg);
	protected:


		unsigned int m_Socket; // SOCKET is unsigned int
		char m_pszAddr[4096] = { 0 };
		unsigned short m_usPort = { 0 };

		void* m_pOutgoingMessagesList = nullptr;
		void* m_pOutgoingMessagesLock = nullptr;

		void* m_pMessagesList = nullptr;
		void* m_pMessagesLock = nullptr;

		void* m_pDeletionLock = nullptr;
	private:
	};

	class CRawNetworkMessage
	{
	public:
		CRawNetworkMessage(void* pData, size_t nSize, int nMessageType) : m_nMessageType(nMessageType), m_nDataSize(nSize)
		{
			m_pData = malloc(nSize + sizeof(MessageHeader_t));
			if (m_pData) { 
				memcpy((char*)m_pData + sizeof(MessageHeader_t), pData, nSize); 
				MessageHeader_t* pHeader = reinterpret_cast<MessageHeader_t*>(m_pData);
				*pHeader = MessageHeader_t();
				pHeader->m_nMessageSize = nSize;
				pHeader->m_nMessageType = nMessageType;
			}
		}
		void FreeMsg() { if (m_pData) { free(m_pData); }; delete this; }

		void* GetRawData() { return (char*)m_pData + sizeof(MessageHeader_t); }
		size_t GetDataSize() { return m_nDataSize; }
		int GetMessageType() { return m_nMessageType; }
		size_t GetFullDataSize() { return m_nDataSize + sizeof(MessageHeader_t); }
		void* GetFullData() { return m_pData; }
	private:
		int m_nMessageType = 0;
		void* m_pData = 0;
		size_t m_nDataSize = -1;
	};

	int NET_Initialize();
	int NET_InitializeWinSocks();

	namespace Shared
	{

	}

	namespace TCP
	{
		class CTCPNetworkConnection : public IRawNetworkConnection
		{
		public:
			CTCPNetworkConnection(unsigned int sSocket) { m_Socket = sSocket; }
			~CTCPNetworkConnection() {
				if (m_pDataBuffer) { free(m_pDataBuffer); }
			}
			CTCPNetworkConnection(
				unsigned int sSocket,
				const char* szAddr,
				unsigned short usPort
			) { m_Socket = sSocket; strcpy_s(m_pszAddr, szAddr); m_usPort = usPort;}

			virtual void Update();
			bool bIsMarkedForDeletion() { return m_bMarkedForDeletion; };
			void MarkForDeletion() { m_bMarkedForDeletion = true; }
			void Delete() { delete this; }
		protected:
			void UpdateIncoming();
			void UpdateOutgoing();

			bool m_bMarkedForDeletion = false;
		private:
			void* m_pDataBuffer = nullptr;
			int m_nCurrentBufferSize = 0;
			int m_nCurrentBytesRecieved = 0;
			bool m_bCurrentRecievingMessageData = 0;
			MessageHeader_t m_PreviousMessageHeader;
		};

		class CTCPNetworkServer
		{
		public:
			CTCPNetworkServer(const char* szPort);
			CTCPNetworkServer();
			~CTCPNetworkServer();

			bool OpenPort(const char* szPort);

			void ConnectionsFrame();
			void ClientFrame();

			void Frame() { ConnectionsFrame(); ClientFrame(); }

			CTCPNetworkConnection** GetClients(unsigned int& nReturned); 
			void FreeClients(CTCPNetworkConnection** pArray);

			void LockClients();
			void UnlockClients();

		protected:
			unsigned int m_ListeningSocket = -1;
			void* m_pClients = nullptr;
			void* m_pDeletables = nullptr;
			void* m_pDeleteLock = nullptr;
			void* m_pDeletionListLock = nullptr;
			void* m_pClientLock = nullptr;

		private:
			uint32_t m_ConfigOptions[_k_GoldNetCfg_CONFIG_MAX] = { 0 };
		};

		_Ret_maybenull_ IRawNetworkConnection* NET_OpenTCPConnection
		(
			_In_z_ const char* szAddr,
			_In_z_ const char* szPort
		);

		void NET_OpenTCPConnectionAsync(
			_In_z_ const char* szAddr,
			_In_z_ const char* szPort,
			_In_ ConnectionCreationCallbackFunc_t fnCallback
		);

		bool NET_OpenTCPListeningSocket
		(
			_In_z_ const char* szPort
		);

	}

	namespace UDP
	{

	}
}

