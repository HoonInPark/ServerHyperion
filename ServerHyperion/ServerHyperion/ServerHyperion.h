#pragma once

#include "ServerIOCP.h"
#include "PacketData.h"
#include "Packet.h"

#include <vector>
#include <thread>
#include <mutex>
#include <queue>

#include "object_pool.hpp"

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
		/*
		printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex_, size_);

		PacketData packet;
		packet.Set(clientIndex_, size_, pData_);

		mPacketDataQueue.push_back(packet);
		*/

		lock_guard<mutex> guard(m_Lock);

		Packet* pPack = m_pDymPackPool->new_object();
		
		if (pPack->Read(pData_, size_))
			printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex_, size_);
		else
			printf("[OnReceive] Read Bin Data Failed");

		m_pPackQ->push(pPack);
	}

	void Run(const UINT32 maxClient)
	{
		m_pDymPackPool = new DynamicObjectPool<Packet>(60);
		m_pPackQ = new queue<Packet*>();

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

		delete m_pPackQ;
		delete m_pDymPackPool;
	}

private:
	void ProcessPacket()
	{
		while (mIsRunProcessThread)
		{
			/*
			auto packetData = DequePacketData();
			if (packetData.GetSize() != 0)
			{
				printf("pos x : %f, pos y : %f, pos z : %f, rot x : %f, rot y : %f, rot z : %f", 
					packetData.GetPosX(),
					packetData.GetPosY(),
					packetData.GetPosZ(),
					packetData.GetRotX(),
					packetData.GetRotY(),
					packetData.GetRotZ());

				//SendMsg(packetData.SessionIndex, packetData.DataSize, packetData.pPacketData); // code made for echo server, unused for hyperion prj
			}
			else
			{
				//this_thread::sleep_for(chrono::milliseconds(1)); // made it wait, not sleeping for arbitrary time
			}
			*/

			if (auto pPack = DeQPack())
			{
				printf("pos x : %f, pos y : %f, pos z : %f, rot x : %f, rot y : %f, rot z : %f",
					pPack->GetPosX(),
					pPack->GetPosY(),
					pPack->GetPosZ(),
					pPack->GetRotX(),
					pPack->GetRotY(),
					pPack->GetRotZ());

				m_pDymPackPool->delete_object(pPack);
			}
			else
			{
				this_thread::sleep_for(chrono::milliseconds(1)); // made it wait, not sleeping for arbitrary time
			}
		}
	}

	/*
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
	*/

	Packet* DeQPack()
	{
		lock_guard<mutex> guard(m_Lock);

		if (m_pPackQ->empty()) return nullptr;

		auto pPack =  m_pPackQ->front();
		m_pPackQ->pop();

		return pPack;
	}

	bool mIsRunProcessThread = false;

	thread mProcessThread;

	mutex m_Lock;
	
	DynamicObjectPool<Packet>* m_pDymPackPool{ nullptr };
	queue<Packet*>* m_pPackQ{ nullptr };
};
