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
	virtual ~ServerHyperion()
	{
		delete m_pPackPool;
		delete m_pPackQ;
	}

	virtual void OnConnect(const UINT32 clientIndex_) override
	{
		printf("[OnConnect] 클라이언트: Index(%d)\n", clientIndex_);

		// send init packet to client

		unique_ptr<Packet> pPack = nullptr;
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
		unique_ptr<Packet> pPack = nullptr;
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
		unique_ptr<Packet> pPack = nullptr;
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
		for (int i = 0; i < PUBLIC_PACK_POOL_SIZE; ++i)
		{
			auto pPacket = make_unique<Packet>();
			m_pPackPool->enqueue(pPacket);
		}

		m_pPackQ = new StlCircularQueue< Packet >(PUBLIC_PACK_POOL_SIZE);

		m_bIsRunProcThread = true;
		m_ProcThread = thread( // lambda bind here use onlu a thread
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

		if (m_ProcThread.joinable())
		{
			m_ProcThread.join();
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
		unique_ptr<Packet> pPack = nullptr;

		char* pStart = nullptr;
		UINT8 Size;

		while (m_bIsRunProcThread)
		{
			if (!m_pPackQ->dequeue(pPack))
			{
				this_thread::sleep_for(chrono::milliseconds(1));
				continue;
			}

			Size = pPack->Write(pStart);
			
			switch (pPack->GetMsgType())
			{
			case MsgType::MSG_INIT:
			{
				for (const auto ConnCliInfo : m_ConnCliInfos)
				{
					if (SESSION_STATUS::DISCONN == ConnCliInfo.second->GetStatus()) continue;
					ConnCliInfo.second->SendMsg(Size, pStart);
				}

				break;
			}
			case MsgType::MSG_GAME:
			{
				for (const auto ConnCliInfo : m_ConnCliInfos)
				{
					// TODO : IsInited() branch must be deleted for performance of game loop 
					if (SESSION_STATUS::INITED != ConnCliInfo.second->GetStatus()) continue;
					// TODO : this branch is gonna be deleted when server-authoritative model is implemented
					if (pPack->GetSessionIdx() != ConnCliInfo.first)
					{
						ConnCliInfo.second->SendMsg(Size, pStart);
					}
				}

				break;
			}
			case MsgType::MSG_CLOSE:
			{
				for (const auto ConnCliInfo : m_ConnCliInfos)
				{
					if (SESSION_STATUS::INITED != ConnCliInfo.second->GetStatus()) continue;
					ConnCliInfo.second->SendMsg(Size, pStart);
				}

				break;
			}
			default:
			{
				assert(true);
				break;
			}
			}

			m_pPackPool->enqueue(pPack);
		}
	}

	bool						m_bIsRunProcThread = false;

	thread						m_ProcThread;

	StlCircularQueue<Packet>*	m_pPackPool{ nullptr };
	StlCircularQueue<Packet>*	m_pPackQ{ nullptr };

};
