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

const UINT32 MAX_SOCK_RECVBUF = 256;	// ���� ������ ũ��
const UINT32 MAX_SOCK_SENDBUF = 4096;	// ���� ������ ũ��
const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;


enum class IOOperation
{
	//INIT,
	ACCEPT,
	RECV,
	SEND
};

//WSAOVERLAPPED����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
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

	WSAOVERLAPPED	m_wsaOverlapped;		//Overlapped I/O����ü
	WSABUF			m_wsaBuf;				//Overlapped I/O�۾� ����
	IOOperation		m_eOperation;			//�۾� ���� ����
	UINT32			SessionIndex = 0;
};

