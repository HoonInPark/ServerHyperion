#pragma once

#include "ServerIOCP.h"
#include "Packet.h"
#include "PacketSampler.h"

#include <vector>
#include <thread>

using namespace std;

#define PUBLIC_PACK_POOL_SIZE 2048
#define GONNA_SAMPLE 0


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
			printf("[OnReceive] : PackPool Error\n");
			return;
		}

		if (pPack->Read(pData_, size_))
		{
			//printf("[OnReceive] CliIdx : %d,  X : %f, Y : %f, Z : %f \n", pPack->GetSessionIdx(), pPack->GetPosX(), pPack->GetPosY(), pPack->GetPosZ());
#if _DEBUG && GONNA_SAMPLE
			m_pPackSampler->Sample(pData_, size_);
#endif

			m_pPackQ->enqueue(pPack);
		}
		else printf("[OnReceive] Read Bin Data Failed");
	}

	void Run(const UINT32 maxClient)
	{
#if _DEBUG && GONNA_SAMPLE
		printf("[Run] WARING : Packet Sampler Initiated");
		m_pPackSampler = make_unique<PacketSampler>();
#endif

		m_pPackQ = new StlCircularQueue<Packet>(PUBLIC_PACK_POOL_SIZE);
		m_pPackPool = new StlCircularQueue<Packet>(PUBLIC_PACK_POOL_SIZE);
		for (int i = 0; i < PUBLIC_PACK_POOL_SIZE; ++i)
		{
			auto pPacket = make_unique<Packet>();
			m_pPackPool->enqueue(pPacket);
		}

		m_pSafeEraseQ = new StlCircularQueue<UINT32>(maxClient);
		m_pSafeErasePool = new StlCircularQueue<UINT32>(maxClient);
		for (int i = 0; i < maxClient; ++i)
		{
			auto pSessIdx = make_unique<UINT32>(0);
			m_pSafeErasePool->enqueue(pSessIdx);
		}

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
#if _DEBUG && GONNA_SAMPLE
		m_pPackSampler->WriteToFile();
#endif

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
			case MsgType::MSG_NONE:
			{
				assert(true);
				break;
			}
			case MsgType::MSG_INIT:
			{
				/*
				for (const auto& ConnCliInfo : m_ConnCliInfos)
				{
					// we don't need this brnach 
					// 'cause switch-case logic in io thread guarantees the member of ConnCliInfos map
					//if (SESSION_STATUS::DISCONN == ConnCliInfo.second->GetStatus()) continue;
					ConnCliInfo.second->SendMsg(Size, pStart);
				}
				*/

				UINT32 SessIdx = pPack->GetSessionIdx();
				m_ConnCliInfos[SessIdx]->BindSpawnMsg([this, SessIdx]()
					{
						unique_ptr<Packet> pPack = nullptr;
						if (!m_pPackPool->dequeue(pPack))
						{
							printf("[SendCompleted()] : PackPool Error");
							return;
						}

						pPack->SetMsgType(MsgType::MSG_SPAWN);
						pPack->SetSessionIdx(SessIdx);

						m_pPackQ->enqueue(pPack);
					});
				m_ConnCliInfos[SessIdx]->SendMsg(Size, pStart);

				break;
			}
			case MsgType::MSG_SPAWN:
			{
				for (const auto& ConnCliInfo : m_ConnCliInfos)
				{
					if (SESSION_STATUS::ST_SPAWNED != ConnCliInfo.second->GetStatus()) continue;
					ConnCliInfo.second->SendMsg(Size, pStart);
				}

				// TODO : 반드시! 모든 세션들이 스폰 메시지를 전송완료했다는 신호를 받은 뒤 아래 동작을 하도록 변경돼야 한다. 
				// TODO : 그러러면 전체적인 구조의 수정이 불가피.
				m_ConnCliInfos[pPack->GetSessionIdx()]->SetStatus(SESSION_STATUS::ST_SPAWNED);

				break;
			}
			case MsgType::MSG_GAME:
			{
				for (const auto& ConnCliInfo : m_ConnCliInfos)
				{
					if (SESSION_STATUS::ST_SPAWNED != ConnCliInfo.second->GetStatus()) continue;
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
				for (const auto& ConnCliInfo : m_ConnCliInfos)
				{
					if (SESSION_STATUS::ST_SPAWNED != ConnCliInfo.second->GetStatus()) continue;
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

			unique_ptr<UINT32> pSafeEraseElem;
			while (m_pSafeEraseQ->dequeue(pSafeEraseElem))
			{
				auto it = m_ConnCliInfos.find(*pSafeEraseElem);
				if (it != m_ConnCliInfos.end())
					m_ConnCliInfos.erase(*pSafeEraseElem);

				m_pSafeErasePool->enqueue(pSafeEraseElem);
			}

			m_pPackPool->enqueue(pPack);
		}
	}

	bool						m_bIsRunProcThread = false;

	thread						m_ProcThread;

	StlCircularQueue<Packet>*	m_pPackPool{ nullptr };
	StlCircularQueue<Packet>*	m_pPackQ{ nullptr };

#if _DEBUG && GONNA_SAMPLE
	unique_ptr<PacketSampler> m_pPackSampler;
#endif

};
