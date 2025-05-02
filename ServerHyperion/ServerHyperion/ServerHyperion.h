#pragma once

#include "ServerIOCP.h"
#include "GlobalBase.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>

using namespace std;

class ServerHyperion : public IOCPServer
{
public:
	ServerHyperion() = default;
	virtual ~ServerHyperion() = default;

	virtual void OnConnect(const UINT32 clientIndex_) override
	{
		printf("[OnConnect] 클라이언트: Index(%d)\n", clientIndex_);
	}

	virtual void OnClose(const UINT32 clientIndex_) override
	{
		printf("[OnClose] 클라이언트: Index(%d)\n", clientIndex_);
	}

	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) override
	{
		printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex_, size_);

		PacketData packet;
		packet.Set(clientIndex_, size_, pData_);

		lock_guard<mutex> guard(mLock);
		mPacketDataQueue.push_back(packet);
	}

	void Run(const UINT32 maxClient)
	{
		mIsRunProcessThread = true;
		mProcessThread = thread( // lambda bind here use onlu a thread
			[this]()
			{
				ProcessPacket();
			}
		);

		StartServer(maxClient);
	}

	void End()
	{
		mIsRunProcessThread = false;

		if (mProcessThread.joinable())
		{
			mProcessThread.join();
		}

		DestroyThread();
	}

private:
	void ProcessPacket()
	{
		while (mIsRunProcessThread)
		{
			auto packetData = DequePacketData();
			if (packetData.DataSize != 0)
			{
				SendMsg(packetData.SessionIndex, packetData.DataSize, packetData.pPacketData); // code made for echo server, unused for hyperion prj
			}
			else
			{
				//this_thread::sleep_for(chrono::milliseconds(1)); // made it wait, not sleeping for arbitrary time
			}
		}
	}

	PacketData DequePacketData()
	{
		PacketData packetData;

		lock_guard<mutex> guard(mLock);
		if (mPacketDataQueue.empty())
		{
			return PacketData();
		}

		packetData.Set(mPacketDataQueue.front());

		mPacketDataQueue.front().Release();
		mPacketDataQueue.pop_front();

		return packetData;
	}

	bool mIsRunProcessThread = false;

	thread mProcessThread;

	mutex mLock;
	deque<PacketData> mPacketDataQueue;
	//deque <shared_ptr< PacketData >> mPacketDataQueue;
};