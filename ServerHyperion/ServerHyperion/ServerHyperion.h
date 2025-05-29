#pragma once

#include "ServerIOCP.h"
#include "Packet.h"

#include <vector>
#include <thread>
#include <mutex>
#include <queue>

#include "ObjPool.h"

using namespace std;

#define PUBLIC_PACK_POOL_SIZE 300

class ServerHyperion : public IOCPServer
{
public:
	ServerHyperion() = default;
	virtual ~ServerHyperion() = default;

	virtual void OnConnect(const UINT32 clientIndex_) override
	{
		printf("[OnConnect] 클라이언트: Index(%d)\n", clientIndex_);

		auto pClient = GetClientInfo(clientIndex_);
		/*
		Packet PackTmp;
		PackTmp.SetSessionIdx(clientIndex_);
		char* pStart = nullptr;
		UINT8 Size = PackTmp.Write(pStart);
		SendMsg(clientIndex_, Size, pStart, IOOperation::INIT);
		*/
	}

	virtual void OnClose(const UINT32 clientIndex_) override
	{
		printf("[OnClose] 클라이언트: Index(%d)\n", clientIndex_);
	}

	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) override
	{
		shared_ptr<Packet> pPack = m_pPackPool->Acquire();

		m_Lock.lock();
		if (!pPack)
		{
			m_Lock.unlock();
			return;
		}

		if (pPack->Read(pData_, size_))
		{
			//printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex_, size_);

			m_pPackQ->push(pPack);
		}
		else printf("[OnReceive] Read Bin Data Failed");

		m_Lock.unlock();
	}

	void Run(const UINT32 maxClient)
	{
		m_pPackPool = new ObjPool<Packet>(PUBLIC_PACK_POOL_SIZE);
		m_pPackQ = new queue<shared_ptr< Packet >>();

		mIsRunProcessThread = true;
		mProcessThread = thread( // lambda bind here use onlu a thread
			[this]()
			{
				ProcPack();
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
	void ProcPack()
	{
		Packet CachePack;
		shared_ptr<Packet> pPack = nullptr;

		char* pStart = nullptr;
		UINT8 Size;

		while (mIsRunProcessThread)
		{
			pPack = nullptr;
			m_Lock.lock();

			if (m_pPackQ->empty())
			{
				m_Lock.unlock();
				this_thread::sleep_for(chrono::milliseconds(1)); // made it wait, not sleeping for arbitrary time

				continue;
			}

			pPack = m_pPackQ->front();
			CachePack.CacheWrite(pPack);
			m_pPackQ->pop();

			m_pPackPool->Return(pPack);

			m_Lock.unlock();
			/*
			printf("pos x : %f, pos y : %f, pos z : %f, rot x : %f, rot y : %f, rot z : %f",
				CachePack.GetPosX(),
				CachePack.GetPosY(),
				CachePack.GetPosZ(),
				CachePack.GetRotX(),
				CachePack.GetRotY(),
				CachePack.GetRotZ());
			*/

			Size = CachePack.Write(pStart);

			for (auto cli : mClientInfos)
			{
				if (cli->IsConnected() == false) continue;
				if (cli->GetIndex() == CachePack.GetSessionIdx())
				{
					SendMsg(CachePack.GetSessionIdx(), Size, pStart);
				}
			}
		}
	}

	bool mIsRunProcessThread = false;

	thread mProcessThread;

	mutex m_Lock;

	ObjPool<Packet>* m_pPackPool{ nullptr };
	queue <shared_ptr< Packet >>* m_pPackQ{ nullptr };
};
