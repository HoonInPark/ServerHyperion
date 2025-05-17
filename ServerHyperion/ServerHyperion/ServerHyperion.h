#pragma once

#include "ServerIOCP.h"
#include "PacketData.h"
#include "Packet.h"

#include <vector>
#include <thread>
#include <mutex>
#include <queue>

#include "ObjPool.h"

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
		shared_ptr<Packet> pPack = nullptr;

		for (;;)
		{
			if (!m_pPackPool->Acquire(pPack))
			{
				this_thread::sleep_for(chrono::milliseconds(1));
				continue;
			}

			if (pPack->Read(pData_, size_))
			{
				printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex_, size_);

				m_pPackQ->push(pPack);
			}
			else printf("[OnReceive] Read Bin Data Failed");

			break;
		}
	}

	void Run(const UINT32 maxClient)
	{
		m_pPackPool = new ObjPool<Packet>(60);
		m_pPackQ = new concurrent_queue<shared_ptr< Packet >>();

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
		delete m_pPackPool;
	}

private:
	void ProcessPacket()
	{
		shared_ptr<Packet> pPack = nullptr;

		while (mIsRunProcessThread)
		{
			if (m_pPackQ->try_pop(pPack))
			{
				printf("pos x : %f, pos y : %f, pos z : %f, rot x : %f, rot y : %f, rot z : %f",
					pPack->GetPosX(),
					pPack->GetPosY(),
					pPack->GetPosZ(),
					pPack->GetRotX(),
					pPack->GetRotY(),
					pPack->GetRotZ());

				m_pPackPool->Return(pPack);
			}
			else
			{
				this_thread::sleep_for(chrono::milliseconds(1)); // made it wait, not sleeping for arbitrary time
			}
		}
	}

	bool mIsRunProcessThread = false;

	thread mProcessThread;

	mutex m_Lock;

	ObjPool<Packet>* m_pPackPool;
	concurrent_queue <shared_ptr< Packet >>* m_pPackQ{ nullptr };
};
