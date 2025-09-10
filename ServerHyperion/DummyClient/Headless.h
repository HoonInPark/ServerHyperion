#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <WS2tcpip.h> 
#include <condition_variable>

#include "PacketSampler.h"
#include "Packet.h"
#include "Define.h"

#pragma comment(lib, "ws2_32.lib")

constexpr const char* SERVER_IP   = "127.0.0.1";
constexpr int         SERVER_PORT = 11021;

class Headless
{
public:
    Headless() : m_hIOCP(NULL), m_hSocket(INVALID_SOCKET) {}
    ~Headless() { Cleanup(); }

    bool Connect(const char* ip, int port) 
    {
        m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_hSocket == INVALID_SOCKET) return false;

        SOCKADDR_IN addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (InetPtonA(AF_INET, ip, &addr.sin_addr) <= 0) return false;

        if (connect(m_hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;

        m_Worker = thread(&Headless::WorkerThread, this);
        return true;
    }

    void WaitIfNotInited()
    {
        unique_lock<mutex> Lock(m_ConVarLock);
        m_ConVar.wait(Lock, [this]
            {
                return m_SessIdx != 0xFFFFFFFF; // if false, it sleeps
            });
    }

    void Send(const char* data, size_t len) 
    {
        int ret = send(m_hSocket, data, (int)len, 0);
        if (ret == SOCKET_ERROR) 
        {
            cerr << "send() failed\n";
        }
    }

    void WorkerThread() 
    {
        char buf[1024];
        Packet Pack;

        while (true) 
        {
            int recvLen = recv(m_hSocket, buf, sizeof(buf), 0);
            Pack.Read(buf, recvLen);

            if (MsgType::MSG_INIT == Pack.GetMsgType())
            {
                if (0xFFFFFFFF == m_SessIdx)
                {
                    m_SessIdx = Pack.GetSessIdx();

                    unique_lock<mutex> Lock(m_ConVarLock);
                    m_ConVar.notify_all();
                }
            }

            if (recvLen <= 0) 
            {
                break; // 연결 끊김
            }
            // 서버 응답 처리 (원하면 로깅 가능)

            this_thread::sleep_for(chrono::milliseconds(1));
        }
    }

    void Cleanup()
    {
        if (m_hSocket != INVALID_SOCKET)
        {
            closesocket(m_hSocket);
            m_hSocket = INVALID_SOCKET;
        }

        if (m_Worker.joinable()) 
            m_Worker.join();
    }

    inline UINT32 GetSessIdx() { return m_SessIdx; }

private:
    condition_variable  m_ConVar;
    mutex               m_ConVarLock;

    UINT32              m_SessIdx{ 0xFFFFFFFF };

    HANDLE              m_hIOCP;
    SOCKET              m_hSocket;
    thread              m_Worker;
};
