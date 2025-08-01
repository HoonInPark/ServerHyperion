//출처: 강정중님의 저서 '온라인 게임서버'에서
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
		//윈속의 사용을 끝낸다.
		WSACleanup();
	}

	//소켓을 초기화하는 함수
	bool Init(const UINT32 maxIOWorkerThreadCount_)
	{
		WSADATA wsaData;

		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			printf("[에러] WSAStartup()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		//연결지향형 TCP , Overlapped I/O 소켓을 생성
		m_ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == m_ListenSocket)
		{
			printf("[에러] socket()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		MaxIOWorkerThreadCount = maxIOWorkerThreadCount_;

		printf("소켓 초기화 성공\n");
		return true;
	}

	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 소켓을 등록하는 함수
	bool BindandListen(int nBindPort)
	{
		SOCKADDR_IN		stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(nBindPort); //서버 포트를 설정한다.

		// 어떤 주소에서 들어오는 접속이라도 받아들이겠다.
		// (보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
		// 그 주소를 inet_addr함수를 이용해 넣으면 된다.)
		stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		// 위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다. 
		// cIOCompletionPort는 서버의 주소정보를 가지고 있는 소켓이다.
		int nRet = bind(m_ListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (0 != nRet)
		{
			printf("[에러] bind()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		//접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
		//접속대기큐를 5개로 설정 한다. -> 5개 이상의 접속 요청이 들어오면 대기큐에 쌓인다.
		nRet = listen(m_ListenSocket, 5);
		if (0 != nRet)
		{
			printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
			return false;
		}

		//CompletionPort객체 생성 요청을 한다.
		m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxIOWorkerThreadCount);
		if (NULL == m_IOCPHandle)
		{
			printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
			return false;
		}

		auto hIOCPHandle = CreateIoCompletionPort((HANDLE)m_ListenSocket, m_IOCPHandle, (UINT32)0, 0);
		if (nullptr == hIOCPHandle)
		{
			printf("[에러] listen socket IOCP bind 실패 : %d\n", WSAGetLastError());
			return false;
		}

		printf("서버 등록 성공..\n");
		return true;
	}

	//접속 요청을 수락하고 메세지를 받아서 처리하는 함수
	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		//접속된 클라이언트 주소 정보를 저장할 구조체
		bool bRet = CreateWokerThread();
		if (false == bRet) {
			return false;
		}

		printf("서버 시작\n");
		return true;
	}

	//생성되어있는 쓰레드를 파괴한다.
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

	//WaitingThread Queue에서 대기할 쓰레드들을 생성
	bool CreateWokerThread()
	{
		unsigned int uiThreadId = 0;
		//WaingThread Queue에 대기 상태로 넣을 쓰레드들 생성 권장되는 개수 : (cpu개수 * 2) + 1 
		for (int i = 0; i < MaxIOWorkerThreadCount; i++)
		{
			m_IOWorkerThreads.emplace_back([this]() { WokerThread(); });
		}

		printf("WokerThread 시작..\n");
		return true;
	}

	inline shared_ptr<stClientInfo> GetEmptyClientInfo()
	{
		auto RetPtr = *m_ClientInfoPool.begin();
		m_ClientInfoPool.erase(m_ClientInfoPool.begin());

		return RetPtr;
	}

	//Overlapped I/O작업에 대한 완료 통보를 받아 그에 해당하는 처리를 하는 함수
	void WokerThread()
	{
		//CompletionKey를 받을 포인터 변수
		shared_ptr<stClientInfo> pCliInfo = nullptr;
		//함수 호출 성공 여부
		BOOL bSuccess = TRUE;
		//Overlapped I/O작업에서 전송된 데이터 크기
		DWORD dwIoSize = 0;
		//I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
		LPOVERLAPPED lpOverlapped = NULL;

		while (m_IsWorkerRun)
		{
			bSuccess = GetQueuedCompletionStatus(
				m_IOCPHandle,
				&dwIoSize,					// 실제로 전송된 바이트
				(PULONG_PTR)&pCliInfo,		// CompletionKey
				&lpOverlapped,				// Overlapped IO 객체
				INFINITE);					// 대기할 시간

			//사용자 쓰레드 종료 메세지 처리..
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

			//client가 접속을 끊었을때..
			if (FALSE == bSuccess || (0 == dwIoSize && IOOperation::IO_ACCEPT != pOverlappedEx->m_eOperation))
			{
				//printf("socket(%d) 접속 끊김\n", (int)pClientInfo->m_socketClient);
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
					//클라이언트 갯수 증가
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
				printf("Client Index(%d)에서 예외상황\n", pCliInfo->GetIndex());
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

	//클라이언트의 접속을 받기위한 리슨 소켓
	SOCKET		m_ListenSocket{ INVALID_SOCKET };

	//접속 되어있는 클라이언트 수
	int			mClientCnt = 0;

	//IO Worker 스레드
	vector<thread> m_IOWorkerThreads;

	//Accept 스레드
	thread	mAccepterThread;

	//CompletionPort객체 핸들
	HANDLE		m_IOCPHandle{ INVALID_HANDLE_VALUE };

	//작업 쓰레드 동작 플래그
	bool		m_IsWorkerRun{ true };

	//접속 쓰레드 동작 플래그
	bool		m_bIsAccepterRun{ true };

protected:
	unordered_set<shared_ptr<stClientInfo>> m_ClientInfoPool;
	unordered_set<shared_ptr<stClientInfo>> m_ConnectedClientInfos;
};
