#pragma once

#include "ServerIOCP.h"
#include "Packet.h"

#include <vector>
#include <thread>

using namespace std;

#define PUBLIC_PACK_POOL_SIZE 512

/// <summary>
/// in ServerHyperion, we wrote game logics rather than low level network pocess...
/// and this class is especially for project hyperion, as IOCPServer class fits for all projects.
/// </summary>
class ServerHyperion : public IOCPServer
{
public:
	ServerHyperion() = default;
	virtual ~ServerHyperion() = default;

	virtual void OnConnect(const UINT32 clientIndex_) override
	{
		printf("[OnConnect] 클라이언트: Index(%d)\n", clientIndex_);

		// send init packet to client

		shared_ptr<Packet> pPack = nullptr;
		if (!m_pPackPool->dequeue(pPack))
		{
			printf("[OnConnect] : PackPool Error");
			return;
		}

		pPack->SetMsgType(MsgType::MSG_INIT);
		pPack->SetSessionIdx(clientIndex_);

		m_pPackQ->enqueue(pPack);
	}

	virtual void OnClose(const UINT32 clientIndex_) override
	{
		shared_ptr<Packet> pPack = nullptr;
		if (!m_pPackPool->dequeue(pPack))
		{
			printf("[OnConnect] : PackPool Error");
			return;
		}

		pPack->SetMsgType(MsgType::MSG_CLOSE);
		pPack->SetSessionIdx(clientIndex_);

		m_pPackQ->enqueue(pPack);

		printf("[OnClose] : Index(%d)\n", clientIndex_);
	}

	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) override
	{
		shared_ptr<Packet> pPack = nullptr;
		if (!m_pPackPool->dequeue(pPack))
		{
			printf("[OnReceive] : PackPool Error");
			return;
		}

		if (pPack->Read(pData_, size_))
		{
			//printf("[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex_, size_);
			m_pPackQ->enqueue(pPack);
		}
		else printf("[OnReceive] Read Bin Data Failed");
	}

	void Run(const UINT32 maxClient)
	{
		m_pPackPool = new StlCircularQueue<Packet>(PUBLIC_PACK_POOL_SIZE);
		m_pPackQ = new StlCircularQueue< Packet >(PUBLIC_PACK_POOL_SIZE);

		m_bIsRunProcThread = true;
		mProcessThread = thread( // lambda bind here use onlu a thread
			[this]()
			{
				ProcPack();
			}
		);

		StartServer(maxClient);
	}

	virtual void CleanupThread() override
	{
		m_bIsRunProcThread = false;

		if (mProcessThread.joinable())
		{
			mProcessThread.join();
		}

		IOCPServer::CleanupThread();
	}

	virtual void End() override
	{
		IOCPServer::End();
		CleanupThread();
	}

private:
	void ProcPack()
	{
		shared_ptr<Packet> pPack = nullptr;

		char* pStart = nullptr;
		UINT8 Size;

		while (m_bIsRunProcThread)
		{
			if (!m_pPackQ->dequeue(pPack))
			{
				this_thread::sleep_for(chrono::milliseconds(1)); // made it wait, not sleeping for arbitrary time
				continue;
			}

			Size = pPack->Write(pStart);
			
			switch (pPack->GetMsgType())
			{
			case MsgType::MSG_INIT:
			{
				for (const auto ConCliInfo : m_ConnectedClientInfos)
					SendMsg(ConCliInfo.first, Size, pStart);

				m_pPackPool->enqueue(pPack);

				continue;
			}
			case MsgType::MSG_GAME:
			{
				for (const auto ConCliInfo : m_ConnectedClientInfos)
				{
					if (0 == ConCliInfo.second->IsInited()) continue; // 이 if 문 어떻게 없앨 수 없을까?
					if (pPack->GetSessionIdx() != ConCliInfo.first)
					{
						SendMsg(ConCliInfo.first, Size, pStart);
					}
				}

				m_pPackPool->enqueue(pPack);

				continue;
			}
			case MsgType::MSG_CLOSE:
			{
				for (const auto ConCliInfo : m_ConnectedClientInfos)
				{
					if (0 == ConCliInfo.second->IsInited()) continue; // if IsInited is 0, also IsConnected is 0, so skip
					SendMsg(ConCliInfo.first, Size, pStart);
				}

				m_pPackPool->enqueue(pPack);

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

	bool m_bIsRunProcThread = false;

	thread mProcessThread;

	StlCircularQueue<Packet>* m_pPackPool{ nullptr };
	StlCircularQueue<Packet>* m_pPackQ{ nullptr };

};
