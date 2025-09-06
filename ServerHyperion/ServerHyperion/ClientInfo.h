#pragma once

#include "Define.h"
#include "StlCircularQueue.h"
#include <stdio.h>
#include <mutex>


enum SESSION_STATUS
{
	// more its number allocated to each enum, 
	// more it is logically strong

	DISCONN = 0,
	CONN,
	INITED,
};
 
//Ŭ���̾�Ʈ ������ ������� ����ü
class CliInfo
{
public:
	CliInfo(atomic<OverlappedEx*>& _InOvlpdEx)
		: m_pAtomicOvlpdEx(_InOvlpdEx)
	{
		m_SessStatus.store(SESSION_STATUS::DISCONN);

		ZeroMemory(&m_RecvOvlpdEx, sizeof(OverlappedEx));
		m_Socket = INVALID_SOCKET;

		m_pSendDataPool = new StlCircularQueue<OverlappedEx>(64);
		for (int i = 0; i < 64; ++i)
		{
			auto pSendData = make_unique<OverlappedEx>();
			m_pSendDataPool->enqueue(pSendData);
		}

		m_pSendBufQ = new StlCircularQueue<OverlappedEx>(64);
	}

	virtual ~CliInfo()
	{
		delete m_pSendBufQ;
		delete m_pSendDataPool;
	}

	void Init(const UINT32 index, HANDLE iocpHandle_)
	{
		m_Index = index;
		m_IOCPHandle = iocpHandle_;
	}

	inline UINT32 GetIndex() { return m_Index; }
	inline SESSION_STATUS GetStatus() { return m_SessStatus.load(); }
	inline SOCKET GetSock() { return m_Socket; }
	inline UINT64 GetLatestClosedTimeSec() { return m_LatestClosedTimeSec; }
	inline char* RecvBuffer() { return m_RecvBuf; }

	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_)
	{
		m_Socket = socket_;
		m_SessStatus.exchange(SESSION_STATUS::CONN);

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

		m_SessStatus.exchange(SESSION_STATUS::DISCONN);
		m_LatestClosedTimeSec = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch()).count();
		//���� ������ ���� ��Ų��.
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
	}

	void Clear()
	{
		m_pInternOvlpdEx = nullptr;

		unique_ptr<OverlappedEx> pHangoverSendOvlpdEx = nullptr;
		while (m_pSendBufQ->dequeue(pHangoverSendOvlpdEx))
			m_pSendDataPool->enqueue(pHangoverSendOvlpdEx);
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

		ZeroMemory(&m_AcceptContext, sizeof(OverlappedEx));

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
		m_RecvOvlpdEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
		m_RecvOvlpdEx.m_wsaBuf.buf = m_RecvBuf;
		m_RecvOvlpdEx.m_eOperation = IOOperation::IO_RECV;

		int nRet = WSARecv(
			m_Socket,
			&(m_RecvOvlpdEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			&dwFlag,
			(LPWSAOVERLAPPED) & (m_RecvOvlpdEx),
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
		unique_ptr<OverlappedEx> pSendOvlpdEx;
		if (!m_pSendDataPool->dequeue(pSendOvlpdEx))
		{
			printf("[SendMsg] : Error in Client %d", m_Index);
			return false;
		}

		////////////////////////////////////////////////////////////////////////////////
		/// write byte
		////////////////////////////////////////////////////////////////////////////////
		pSendOvlpdEx->Init();
		pSendOvlpdEx->m_wsaBuf.len = _InSize;
		CopyMemory(pSendOvlpdEx->m_wsaBuf.buf, _pInMsg, _InSize);
		pSendOvlpdEx->m_eOperation = IOOperation::IO_SEND;
		////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////

		m_pSendBufQ->enqueue(pSendOvlpdEx);
		// If there are no messages currently in the process of being sent
		if (nullptr == m_pAtomicOvlpdEx.load(memory_order_relaxed))
		{
			if (m_pSendBufQ->dequeue(m_pInternOvlpdEx))
			{
				m_pAtomicOvlpdEx.store(m_pInternOvlpdEx.get(), memory_order_release);
				SendIO(m_pInternOvlpdEx);
			}
			else
			{
				printf("[SendMsg] : Error while dequeue from send buf q, data race is suspected");
				return false;
			}
		}

		return true;
	}

	void SendCompleted(const UINT32 dataSize_)
	{
		//printf("[SendCompleted()] bytes : %d\n", dataSize_);

		char MsgTypeInBuff = m_pInternOvlpdEx->m_wsaBuf.buf[static_cast<int>(Packet::Header::MAX)];
		switch (static_cast<MsgType>(MsgTypeInBuff))
		{
		case MsgType::MSG_NONE:
		{
			printf("[SendCompleted()] : Error\n");

			break;
		}
		case MsgType::MSG_INIT:
		{
			printf("[SendCompleted()] MSG_INIT\n");
			m_SessStatus.exchange(SESSION_STATUS::INITED); // set initialized

			break;
		}
		case MsgType::MSG_GAME:
		{
			break;
		}
		default:
			assert(true);
			break;
		}

		m_pSendDataPool->enqueue(m_pInternOvlpdEx);

		if (m_pSendBufQ->dequeue(m_pInternOvlpdEx))
		{
			m_pAtomicOvlpdEx.exchange(m_pInternOvlpdEx.get(), memory_order_acq_rel);
			SendIO(m_pInternOvlpdEx);
		}
		else
		{
			m_pInternOvlpdEx = nullptr;
			m_pAtomicOvlpdEx.exchange(nullptr, memory_order_release);
		}
	}

private:
	bool SendIO(const unique_ptr<OverlappedEx>& _pInSendOverlappedEx)
	{
		DWORD dwRecvNumBytes = 0;
		int nRet = WSASend(
			m_Socket,
			&(_pInSendOverlappedEx->m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED)_pInSendOverlappedEx.get(), // ����ü�� ���ӵ� �޸� ������ �Ҵ�ȴ� -> ù��° ����� �ּҴ� �� �� ����ü�� ��������. ���� ĳ���� ����. ����...
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
	INT32							m_Index = 0;
	HANDLE							m_IOCPHandle = INVALID_HANDLE_VALUE;

	atomic<SESSION_STATUS>			m_SessStatus;

	UINT64							m_LatestClosedTimeSec = 0;

	SOCKET							m_Socket;			//Cliet�� ����Ǵ� ����

	OverlappedEx					m_AcceptContext;
	char							m_AcceptBuf[64];

	OverlappedEx					m_RecvOvlpdEx;	//IO_RECV Overlapped I/O�۾��� ���� ����	
	char							m_RecvBuf[MAX_SOCK_RECVBUF]; //������ ����

	// m_AliveOvlpdEx always must be changed through m_AtomicOvlpdEx. 
	unique_ptr<OverlappedEx>		m_pInternOvlpdEx; // not to deleted when ref cnt go to zero
	atomic<OverlappedEx*>&			m_pAtomicOvlpdEx;

	StlCircularQueue<OverlappedEx>* m_pSendBufQ;
	StlCircularQueue<OverlappedEx>* m_pSendDataPool;
};
