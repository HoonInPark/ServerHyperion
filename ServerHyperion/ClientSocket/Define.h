#pragma once

#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
#include "Packet.h"

const UINT32 MAX_SOCK_RECVBUF = 256;	// 소켓 버퍼의 크기
const UINT32 MAX_SOCK_SENDBUF = 4096;	// 소켓 버퍼의 크기
const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;


enum class IOOperation
{
	//INIT,
	ACCEPT,
	RECV,
	SEND
};

//WSAOVERLAPPED구조체를 확장 시켜서 필요한 정보를 더 넣었다.
struct stOverlappedEx
{
	stOverlappedEx()
	{
		m_wsaBuf.buf = new char[Packet::GetMaxPackByteSize()];
		Init();
	}

	void Init() // for init after Acquired from obj pool
	{
		ZeroMemory(&m_wsaOverlapped, sizeof(WSAOVERLAPPED));
		ZeroMemory(m_wsaBuf.buf, Packet::GetMaxPackByteSize());
		m_wsaBuf.len = 0;
		ZeroMemory(&m_eOperation, sizeof(IOOperation));
		SessionIndex = 0;
	}

	WSAOVERLAPPED	m_wsaOverlapped;		//Overlapped I/O구조체
	WSABUF			m_wsaBuf;				//Overlapped I/O작업 버퍼
	IOOperation		m_eOperation;			//작업 동작 종류
	UINT32			SessionIndex = 0;
};

