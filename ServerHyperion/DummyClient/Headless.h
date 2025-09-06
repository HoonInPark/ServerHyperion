#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <WS2tcpip.h> 
#include "PacketSampler.h"

#pragma comment(lib, "ws2_32.lib")

constexpr const char* SERVER_IP   = "127.0.0.1";
constexpr int         SERVER_PORT = 11021;

class Headless
{
public:
    Headless() : m_hIOCP(NULL), m_hSocket(INVALID_SOCKET) {}
    ~Headless() { Cleanup(); }

    bool Init()
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            cerr << "WSAStartup failed\n";
            return false;
        }
        return true;
    }

    bool Connect(const char* ip, int port)
    {
        m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_hSocket == INVALID_SOCKET)
        {
            cerr << "socket() failed\n";
            return false;
        }

        SOCKADDR_IN addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        // ANSI 버전 사용 (ip는 const char* 이므로 변환 불필요)
        if (InetPtonA(AF_INET, ip, &addr.sin_addr) <= 0) {
            cerr << "InetPton failed for ip: " << ip << "\n";
            return false;
        }

        if (connect(m_hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            cerr << "connect() failed\n";
            return false;
        }

        // IOCP 생성
        m_hIOCP = CreateIoCompletionPort((HANDLE)m_hSocket, NULL, (ULONG_PTR)this, 0);
        if (!m_hIOCP)
        {
            cerr << "CreateIoCompletionPort failed\n";
            return false;
        }

        // 수신용 스레드 시작
        m_Worker = thread(&Headless::WorkerThread, this);

        return true;
    }

    void SendLoop(PacketSampler& sampler)
    {
        size_t idx = 0;
        while (true)
        {
            if (idx >= sampler.m_Data.size()) idx = 0;

            size_t len = sampler.m_Meta[idx];
            char* buf  = sampler.m_Data[idx];

            int ret = send(m_hSocket, buf, (int)len, 0);
            if (ret == SOCKET_ERROR)
            {
                cerr << "send failed: " << WSAGetLastError() << "\n";
                break;
            }

            // 30Hz → 33ms 간격
            this_thread::sleep_for(chrono::milliseconds(33));
            idx++;
        }
    }

    void WorkerThread()
    {
        char buf[4096];
        while (true)
        {
            int ret = recv(m_hSocket, buf, sizeof(buf), 0);
            if (ret > 0)
            {
                cout << "[Recv] size=" << ret << "\n";
            }
            else if (ret == 0)
            {
                cout << "Server closed connection\n";
                break;
            }
            else
            {
                cerr << "recv error: " << WSAGetLastError() << "\n";
                break;
            }
        }
    }

    void Cleanup()
    {
        if (m_hSocket != INVALID_SOCKET)
        {
            closesocket(m_hSocket);
            m_hSocket = INVALID_SOCKET;
        }
        if (m_Worker.joinable()) m_Worker.join();
        WSACleanup();
    }

private:
    HANDLE m_hIOCP;
    SOCKET m_hSocket;
    thread m_Worker;
};
