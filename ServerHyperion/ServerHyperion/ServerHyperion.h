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

		// send init packet to client
		m_Lock.lock();

		shared_ptr<Packet> pPack = m_pPackPool->Acquire();
		if (!pPack)
		{
			m_Lock.unlock();
			printf("[OnConnect] : PackPool Error");
			return;
		}

		pPack->SetMsgType(MsgType::MSG_INIT);
		pPack->SetSessionIdx(clientIndex_);

		m_pPackQ->push(pPack);

		m_Lock.unlock();
	}

	virtual void OnClose(const UINT32 clientIndex_) override
	{
		m_Lock.lock();

		shared_ptr<Packet> pPack = m_pPackPool->Acquire();
		if (!pPack)
		{
			m_Lock.unlock();
			printf("[OnConnect] : PackPool Error");
			return;
		}

		pPack->SetMsgType(MsgType::MSG_CLOSE);
		pPack->SetSessionIdx(clientIndex_);

		m_pPackQ->push(pPack);

		m_Lock.unlock();

		printf("[OnClose] : Index(%d)\n", clientIndex_);
	}

	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) override
	{
		m_Lock.lock();

		shared_ptr<Packet> pPack = m_pPackPool->Acquire();
		if (!pPack)
		{
			m_Lock.unlock();
			printf("[OnReceive] : PackPool Error");
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
		shared_ptr<Packet> pPack = nullptr;

		char* pStart = nullptr;
		UINT8 Size;

		while (mIsRunProcessThread)
		{
			m_Lock.lock();

			if (m_pPackQ->empty())
			{
				m_Lock.unlock();
				this_thread::sleep_for(chrono::milliseconds(1)); // made it wait, not sleeping for arbitrary time

				continue;
			}

			pPack = m_pPackQ->front();
			m_pPackQ->pop();
			m_Lock.unlock();

			Size = pPack->Write(pStart);

			switch (pPack->GetMsgType())
			{
			case MsgType::MSG_INIT:
			{
				for (int i = 0; i < m_ClientInfos.size(); ++i)
				{
					if (0 == m_ClientInfos[i]->IsConnected()) continue; // if IsInited is 0, also IsConnected is 0, so skip
					SendMsg(i, Size, pStart);
				}

				m_Lock.lock();
				m_pPackPool->Return(pPack);
				m_Lock.unlock();

				continue;
			}
			case MsgType::MSG_GAME:
			{
				for (int i = 0; i < m_ClientInfos.size(); ++i)
				{
					if (0 == m_ClientInfos[i]->IsInited()) continue; // if IsInited is 0, also IsConnected is 0, so skip
					if (pPack->GetSessionIdx() != i)
					{
						SendMsg(i, Size, pStart);
					}
				}

				m_Lock.lock();
				m_pPackPool->Return(pPack);
				m_Lock.unlock();

				continue;
			}
			case MsgType::MSG_CLOSE:
			{
				for (int i = 0; i < m_ClientInfos.size(); ++i)
				{
					if (0 == m_ClientInfos[i]->IsInited()) continue; // if IsInited is 0, also IsConnected is 0, so skip
					SendMsg(i, Size, pStart);
				}

				m_Lock.lock();
				m_pPackPool->Return(pPack);
				m_Lock.unlock();

				continue;
			}
			default:
			{
				printf("[ProcPack] unknown msg type ERROR!!!");
				break;
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
