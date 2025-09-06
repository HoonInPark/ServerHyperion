#include <iostream>
#include "Headless.h"

int main()
{
    PacketSampler sampler;
    sampler.ReadFile(); // 샘플 데이터 읽기

    Headless client;
    if (!client.Init()) return 1;
    if (!client.Connect(SERVER_IP, SERVER_PORT)) return 1;

    client.SendLoop(sampler); // 패킷 송신 루프
    return 0;
}
