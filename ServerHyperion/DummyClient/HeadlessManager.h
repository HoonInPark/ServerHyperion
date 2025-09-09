#pragma once

#include "Headless.h"
#include "Packet.h"
#include "PacketSampler.h"

class HeadlessManager 
{
public:
    HeadlessManager(const char* _InIp, int _InServerPort, PacketSampler* _InSampler)
        : m_Ip          (_InIp)
        , m_ServerPort  (_InServerPort)
        , m_Sampler     (_InSampler)
    {
    }

    void Run(int numClients);

private:
    const char* m_Ip;
    int m_ServerPort;
    PacketSampler* m_Sampler;
    vector<unique_ptr<Headless>> m_Clients;
};

void HeadlessManager::Run(int NumClients) 
{
    for (int i = 0; i < NumClients; ++i) 
    {
        auto client = make_unique<Headless>();
        if (client->Connect(m_Ip, m_ServerPort)) 
        {
            m_Clients.push_back(move(client));
        }
    }

    cout << "Connected " << m_Clients.size() << " clients.\n";
    if (m_Clients.size() < NumClients)
    {
        printf("Connection Errors in some Clients...");
        return;
    }

    Packet PackBuf;
    char* CharBuf = nullptr;

    // 30fps 루프
    while (true) 
    {
        for (size_t i = 0; i < m_Sampler->m_Data.size(); ++i)
        {
            for (size_t j = 0; j < m_Clients.size(); ++j)
            {
                m_Clients[j]->WaitIfNotInited();

                PackBuf.Read(m_Sampler->m_Data[i], m_Sampler->m_Meta[i]);

                PackBuf.SetSessionIdx(m_Clients[j]->GetSessIdx());

                // if it has x, y, z data in its body, 
                //  u must re-assign its val
                if (PackBuf.GetHeader()[2])
                {
                    PackBuf.SetPosX(PackBuf.GetPosX() + (70.0 * (int)(j / 10)));
                    PackBuf.SetPosY(PackBuf.GetPosY() + (70.0 * (int)(j % 10)));
                }

                size_t PackLen = PackBuf.Write(CharBuf);
                //printf("Session Index : %d\n", m_Clients[j]->GetSessIdx());
                m_Clients[j]->Send(CharBuf, PackLen);
            }
            
            this_thread::sleep_for(chrono::milliseconds(33)); // 약 30회/초
        }
    }
}
