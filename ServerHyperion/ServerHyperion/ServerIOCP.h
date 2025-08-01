//��ó: �����ߴ��� ���� '�¶��� ���Ӽ���'����
#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock.lib")

#include "ClientInfo.h"
#include "Define.h"
#include <thread>
#include <vector>
#include <unordered_set>
#include <coroutine>

class IOCPServer
{
public:
	IOCPServer(void) {}

	virtual ~IOCPServer(void)
	{
		//������ ����� ������.
		WSACleanup();
	}

	//������ �ʱ�ȭ�ϴ� �Լ�
	bool Init(const UINT32 maxIOWorkerThreadCount_)
	{
		WSADATA wsaData;

		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			printf("[����] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		//���������� TCP , Overlapped I/O ������ ����
		m_ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == m_ListenSocket)
		{
			printf("[����] socket()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		MaxIOWorkerThreadCount = maxIOWorkerThreadCount_;

		printf("���� �ʱ�ȭ ����\n");
		return true;
	}

	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� ������ ����ϴ� �Լ�
	bool BindandListen(int nBindPort)
	{
		SOCKADDR_IN		stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(nBindPort); //���� ��Ʈ�� �����Ѵ�.

		// � �ּҿ��� ������ �����̶� �޾Ƶ��̰ڴ�.
		// (���� ������� �̷��� �����Ѵ�. ���� �� �����ǿ����� ������ �ް� �ʹٸ�
		// �� �ּҸ� inet_addr�Լ��� �̿��� ������ �ȴ�.)
		stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		// ������ ������ ���� �ּ� ������ cIOCompletionPort ������ �����Ѵ�. 
		// cIOCompletionPort�� ������ �ּ������� ������ �ִ� �����̴�.
		int nRet = bind(m_ListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (0 != nRet)
		{
			printf("[����] bind()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		//���� ��û�� �޾Ƶ��̱� ���� cIOCompletionPort������ ����ϰ� 
		//���Ӵ��ť�� 5���� ���� �Ѵ�. -> 5�� �̻��� ���� ��û�� ������ ���ť�� ���δ�.
		nRet = listen(m_ListenSocket, 5);
		if (0 != nRet)
		{
			printf("[����] listen()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		//CompletionPort��ü ���� ��û�� �Ѵ�.
		m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxIOWorkerThreadCount);
		if (NULL == m_IOCPHandle)
		{
			printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
			return false;
		}

		auto hIOCPHandle = CreateIoCompletionPort((HANDLE)m_ListenSocket, m_IOCPHandle, (UINT32)0, 0);
		if (nullptr == hIOCPHandle)
		{
			printf("[����] listen socket IOCP bind ���� : %d\n", WSAGetLastError());
			return false;
		}

		printf("���� ��� ����..\n");
		return true;
	}

	//���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		//���ӵ� Ŭ���̾�Ʈ �ּ� ������ ������ ����ü
		bool bRet = CreateWokerThread();
		if (false == bRet) {
			return false;
		}

		printf("���� ����\n");
		return true;
	}

	//�����Ǿ��ִ� �����带 �ı��Ѵ�.
	virtual void CleanupThread()
	{
		m_IsWorkerRun = false;

		for (auto& th : m_IOWorkerThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		this_thread::sleep_for(chrono::seconds(RE_USE_SESSION_WAIT_TIMESEC + 1)); // wait for every detached thread made by CloseSocket(...) func.
	}

	virtual void End()
	{
		CloseHandle(m_IOCPHandle);
		closesocket(m_ListenSocket);
	}

	bool SendMsg(const UINT32 sessionIndex_, const UINT32 _InDataSize, char* _pInData)
	{
		auto pClient = GetClientInfo(sessionIndex_);
		return pClient->SendMsg(_InDataSize, _pInData);
	}

	virtual void OnConnect(const UINT32 clientIndex_) {}
	virtual void OnClose(const UINT32 clientIndex_) {}
	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) {}

private:
	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; ++i)
		{
			auto pCliInfo = make_shared<stClientInfo>();
			pCliInfo->PostAccept(m_ListenSocket);
			m_ClientInfoPool.insert(pCliInfo);
		}
	}

	//WaitingThread Queue���� ����� ��������� ����
	bool CreateWokerThread()
	{
		unsigned int uiThreadId = 0;
		//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
		for (int i = 0; i < MaxIOWorkerThreadCount; i++)
		{
			m_IOWorkerThreads.emplace_back([this]() { WokerThread(); });
		}

		printf("WokerThread ����..\n");
		return true;
	}

	inline shared_ptr<stClientInfo> GetEmptyClientInfo()
	{
		auto RetPtr = *m_ClientInfoPool.begin();
		m_ClientInfoPool.erase(m_ClientInfoPool.begin());

		return RetPtr;
	}

	//Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� �׿� �ش��ϴ� ó���� �ϴ� �Լ�
	void WokerThread()
	{
		//CompletionKey�� ���� ������ ����
		shared_ptr<stClientInfo> pCliInfo = nullptr;
		//�Լ� ȣ�� ���� ����
		BOOL bSuccess = TRUE;
		//Overlapped I/O�۾����� ���۵� ������ ũ��
		DWORD dwIoSize = 0;
		//I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
		LPOVERLAPPED lpOverlapped = NULL;

		while (m_IsWorkerRun)
		{
			bSuccess = GetQueuedCompletionStatus(
				m_IOCPHandle,
				&dwIoSize,					// ������ ���۵� ����Ʈ
				(PULONG_PTR)&pCliInfo,		// CompletionKey
				&lpOverlapped,				// Overlapped IO ��ü
				INFINITE);					// ����� �ð�

			//����� ������ ���� �޼��� ó��..
			if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
			{
				m_IsWorkerRun = false;
				continue;
			}

			if (NULL == lpOverlapped)
			{
				continue;
			}

			auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			//client�� ������ ��������..
			if (FALSE == bSuccess || (0 == dwIoSize && IOOperation::IO_ACCEPT != pOverlappedEx->m_eOperation))
			{
				//printf("socket(%d) ���� ����\n", (int)pClientInfo->m_socketClient);
				CloseSocket(pCliInfo);
				continue;
			}

			switch (pOverlappedEx->m_eOperation)
			{
			case IOOperation::IO_ACCEPT:
			{
				//pCliInfo = GetClientInfo(pOverlappedEx->SessionIndex);
				if (pCliInfo->AcceptCompletion())
				{
					//Ŭ���̾�Ʈ ���� ����
					/*
					++mClientCnt;
					OnConnect(pCliInfo->GetIndex());
					*/
				}
				else
				{
					CloseSocket(pCliInfo, true);
				}

				break;
			}
			case IOOperation::IO_RECV:
			{
				OnReceive(pCliInfo->GetIndex(), dwIoSize, pCliInfo->RecvBuffer());
				pCliInfo->BindRecv();
				break;
			}
			case IOOperation::IO_SEND:
			{
				pCliInfo->SendCompleted(dwIoSize);
				break;
			}
			default:
				printf("Client Index(%d)���� ���ܻ�Ȳ\n", pCliInfo->GetIndex());
				break;
			}
		}
	}

	void CloseSocket(shared_ptr<stClientInfo> pCliInfo, bool bIsForce = false)
	{
		auto clientIndex = pCliInfo->GetIndex();
		pCliInfo->Close(bIsForce);
		OnClose(clientIndex);

		thread([&, pCliInfo]()
			{
				this_thread::sleep_for(chrono::seconds(RE_USE_SESSION_WAIT_TIMESEC));

				if (pCliInfo)
				{
					pCliInfo->PostAccept(m_ListenSocket);
					m_ClientInfoPool.insert(pCliInfo);
				}
			}).detach();
	}

protected:
	stClientInfo* GetClientInfo(const UINT32 sessionIndex)
	{
		//return m_ClientInfos[sessionIndex];
	}


	UINT32 MaxIOWorkerThreadCount{ 0 };

	//Ŭ���̾�Ʈ�� ������ �ޱ����� ���� ����
	SOCKET		m_ListenSocket{ INVALID_SOCKET };

	//���� �Ǿ��ִ� Ŭ���̾�Ʈ ��
	int			mClientCnt = 0;

	//IO Worker ������
	vector<thread> m_IOWorkerThreads;

	//Accept ������
	thread	mAccepterThread;

	//CompletionPort��ü �ڵ�
	HANDLE		m_IOCPHandle{ INVALID_HANDLE_VALUE };

	//�۾� ������ ���� �÷���
	bool		m_IsWorkerRun{ true };

	//���� ������ ���� �÷���
	bool		m_bIsAccepterRun{ true };

protected:
	unordered_set<shared_ptr<stClientInfo>> m_ClientInfoPool;
	unordered_set<shared_ptr<stClientInfo>> m_ConnectedClientInfos;
};
