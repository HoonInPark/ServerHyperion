#include <iostream>
#include "Headless.h"
#include "HeadlessManager.h"

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed\n";
    }

    PacketSampler sampler;
    sampler.ReadFile();

    HeadlessManager mgr("127.0.0.1", 11021, &sampler);
    mgr.Run(2);  // 클라이언트 100개 띄워서 테스트

    WSACleanup();
}
