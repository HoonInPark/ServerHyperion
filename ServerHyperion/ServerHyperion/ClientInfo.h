#pragma once

#include "Define.h"
#include "StlCircularQueue.h"
#include <stdio.h>
#include <mutex>


//Ŭ���̾�Ʈ ������ ������� ����ü
class stClientInfo
{
public:
	stClientInfo(atomic< shared_ptr<stOverlappedEx >>& _InOverlappedEx)
		: m_OverlappedEx(_InOverlappedEx)
	{
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		m_Socket = INVALID_SOCKET;

		m_pSendDataPool = new StlCircularQueue<stOverlappedEx>(64);
		for (int i = 0; i < 64; ++i)
			m_pSendDataPool->enqueue(make_shared<stOverlappedEx>());

		m_pSendBufQ = new StlCircularQueue<stOverlappedEx>(64);
	}

	void Init(const UINT32 index, HANDLE iocpHandle_)
	{
		m_Index = index;
		m_IOCPHandle = iocpHandle_;
	}

	inline UINT32 GetIndex() { return m_Index; }
	inline bool IsConnected() { return m_IsConnected == 1; }
	inline bool IsInited() { return m_IsInited == 1; }
	inline SOCKET GetSock() { return m_Socket; }
	inline UINT64 GetLatestClosedTimeSec() { return m_LatestClosedTimeSec; }
	inline char* RecvBuffer() { return mRecvBuf; }

	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_)
	{
		m_Socket = socket_;
		m_IsConnected = 1;

		Clear();

		//I/O Completion Port��ü�� ������ �����Ų��.
		if (BindIOCompletionPort(iocpHandle_) == false)
		{
			return false;
		}

		return BindRecv();
	}

	void Close(bool bIsForce = false)
	{
		struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

		// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
		if (true == bIsForce)
		{
			stLinger.l_onoff = 1;
		}

		//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
		shutdown(m_Socket, SD_BOTH);

		//���� �ɼ��� �����Ѵ�.
		setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		m_IsConnected = 0;
		m_IsInited = 0;
		m_LatestClosedTimeSec = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch()).count();
		//���� ������ ���� ��Ų��.
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
	}

	void Clear()
	{
	}

	bool PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_ = 0)
	{
		printf_s("PostAccept. client Index: %d\n", GetIndex());

		m_LatestClosedTimeSec = UINT32_MAX;

		m_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP,
			NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == m_Socket)
		{
			printf_s("client Socket WSASocket Error : %d\n", GetLastError());
			return false;
		}

		ZeroMemory(&m_AcceptContext, sizeof(stOverlappedEx));

		DWORD bytes = 0;
		DWORD flags = 0;
		m_AcceptContext.m_wsaBuf.len = 0;
		m_AcceptContext.m_wsaBuf.buf = nullptr;
		m_AcceptContext.m_eOperation = IOOperation::IO_ACCEPT;
		m_AcceptContext.SessionIndex = m_Index;

		if (FALSE == AcceptEx(listenSock_, m_Socket, m_AcceptBuf, 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (m_AcceptContext)))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf_s("AcceptEx Error : %d\n", GetLastError());
				return false;
			}
		}

		return true;
	}

	bool AcceptCompletion()
	{
		printf_s("AcceptCompletion : SessionIndex(%d)\n", m_Index);

		if (OnConnect(m_IOCPHandle, m_Socket) == false)
		{
			return false;
		}

		SOCKADDR_IN		stClientAddr;
		int nAddrLen = sizeof(SOCKADDR_IN);
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		printf("Ŭ���̾�Ʈ ���� : IP(%s) SOCKET(%d)\n", clientIP, (int)m_Socket);

		return true;
	}

	bool BindIOCompletionPort(HANDLE iocpHandle_)
	{
		//socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
		auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
			, iocpHandle_
			, (ULONG_PTR)(this)
			, 0);

		if (hIOCP == INVALID_HANDLE_VALUE)
		{
			printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
			return false;
		}

		return true;
	}

	bool BindRecv()
	{
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		//Overlapped I/O�� ���� �� ������ ������ �ش�.
		mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
		mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
		mRecvOverlappedEx.m_eOperation = IOOperation::IO_RECV;

		int nRet = WSARecv(
			m_Socket,
			&(mRecvOverlappedEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			&dwFlag,
			(LPWSAOVERLAPPED) & (mRecvOverlappedEx),
			NULL);

		//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[����] WSARecv()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	// 1���� �����忡���� ȣ���ؾ� �Ѵ�! 
	// obj pooling must be implemented
	bool SendMsg(const UINT32 _InSize, char* _pInMsg)
	{
		shared_ptr<stOverlappedEx> pSendOverlappedEx;
		if (!m_pSendDataPool->dequeue(pSendOverlappedEx))
		{
			printf("SendMsg Error in Client %d", m_Index);
			return false;
		}

		pSendOverlappedEx->Init();
		pSendOverlappedEx->m_wsaBuf.len = _InSize;
		CopyMemory(pSendOverlappedEx->m_wsaBuf.buf, _pInMsg, _InSize);
		pSendOverlappedEx->m_eOperation = IOOperation::IO_SEND;

		m_pSendBufQ->enqueue(pSendOverlappedEx);
		if (nullptr == m_OverlappedEx.load(memory_order_relaxed))
		{
			shared_ptr<stOverlappedEx> pFirstSendOverlappedEx;
			if (m_pSendBufQ->dequeue(pFirstSendOverlappedEx))
			{
				m_OverlappedEx.exchange(pFirstSendOverlappedEx, memory_order_acq_rel);
				SendIO(pFirstSendOverlappedEx);
			}
		}

		return true;
	}

	void SendCompleted(const UINT32 dataSize_)
	{
		printf("[�۽� �Ϸ�] bytes : %d\n", dataSize_);

		shared_ptr<stOverlappedEx> pSendOverlappedEx = m_OverlappedEx.load(memory_order_relaxed);

		char MsgTypeInBuff = pSendOverlappedEx->m_wsaBuf.buf[static_cast<int>(Packet::Header::MAX)];
		switch (static_cast<MsgType>(MsgTypeInBuff))
		{
		case MsgType::MSG_NONE:
		{
			printf("[SendCompleted] : Error\n");

			break;
		}
		case MsgType::MSG_INIT:
		{
			printf("[SendCompleted()] MSG_INIT\n");
			m_IsInited = 1; // set initialized

			break;
		}
		case MsgType::MSG_GAME:
		{
			break;
		}
		default:
			break;
		}

		m_pSendDataPool->enqueue(pSendOverlappedEx);

		shared_ptr<stOverlappedEx> pNextSendOverlappedEx;
		if (m_pSendBufQ->dequeue(pNextSendOverlappedEx))
		{
			m_OverlappedEx.exchange(pNextSendOverlappedEx, memory_order_acq_rel);
			SendIO(pNextSendOverlappedEx);
		}
		else
		{
			m_OverlappedEx.store(nullptr, memory_order_release);
		}
	}

private:
	bool SendIO(const shared_ptr<stOverlappedEx>  _pInSendOverlappedEx)
	{
		DWORD dwRecvNumBytes = 0;
		int nRet = WSASend(
			m_Socket,
			&(_pInSendOverlappedEx->m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED)_pInSendOverlappedEx.get(),
			NULL);

		//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[����] WSASend()�Լ� ���� : %d\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	bool SetSocketOption()
	{
		/*if (SOCKET_ERROR == setsockopt(mSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
		{
			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
			return false;
		}*/

		int opt = 1;
		if (SOCKET_ERROR == setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
			return false;
		}

		opt = 0;
		if (SOCKET_ERROR == setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
			return false;
		}

		return true;
	}

private:
	INT32 m_Index = 0;
	HANDLE m_IOCPHandle = INVALID_HANDLE_VALUE;

	INT64 m_IsConnected = 0;
	atomic<INT64> m_IsInited = 0; // 0: not initialized, 1: initialized

	UINT64 m_LatestClosedTimeSec = 0;

	SOCKET			m_Socket;			//Cliet�� ����Ǵ� ����

	stOverlappedEx	m_AcceptContext;
	char m_AcceptBuf[64];

	stOverlappedEx	mRecvOverlappedEx;	//IO_RECV Overlapped I/O�۾��� ���� ����	
	char			mRecvBuf[MAX_SOCK_RECVBUF]; //������ ����

	atomic< shared_ptr<stOverlappedEx >>& m_OverlappedEx;

	StlCircularQueue<stOverlappedEx>* m_pSendBufQ;
	StlCircularQueue<stOverlappedEx>* m_pSendDataPool;
};
