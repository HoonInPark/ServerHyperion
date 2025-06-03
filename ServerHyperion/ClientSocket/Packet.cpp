#include "Packet.h"

#include "Define.h"

UINT32 Packet::GetMaxPackByteSize()
{
	UINT32 DataSize = 0;

	DataSize += static_cast<int>(Header::MAX); // Header size
	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
		DataSize += GetSize(static_cast<Header>(i)); // add size of each value

	return DataSize;
}

Packet::Packet()
{
	m_pBinData = new char[GetMaxPackByteSize()];
}

Packet::~Packet()
{
	if (m_pBinData)
		delete[] m_pBinData;

	m_pBinData = nullptr;
}

UINT32 Packet::Write(char*& _pOutStartPt)
{
	_pOutStartPt = m_pBinData; // copy m_pBinData ptr that is already alloced ptr mem

	UINT32 WriteIdx = 0;

	// write header
	bool bHeader = false;
	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
	{
		bHeader = m_Header[i];
		CopyMemory(_pOutStartPt + WriteIdx, &bHeader, sizeof(char));
		WriteIdx++;
	}

	// write body
	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
	{
		if (!m_Header[i]) continue;

		switch (static_cast<Header>(i))
		{
		case Header::MSG_TYPE:
		{
			char MsgTypeVal = static_cast<char>(m_MsgType);

			CopyMemory(_pOutStartPt + WriteIdx, &MsgTypeVal, sizeof(char));
			WriteIdx++;
			break;
		}

		case Header::SESSION_IDX:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_SessionIdx, sizeof(UINT32));
			WriteIdx += sizeof(UINT32);
			break;
		}

		case Header::POS_X:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_PosX, sizeof(double));
			WriteIdx += sizeof(double);
			break;
		}
		case Header::POS_Y:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_PosY, sizeof(double));
			WriteIdx += sizeof(double);
			break;
		}
		case Header::POS_Z:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_PosZ, sizeof(double));
			WriteIdx += sizeof(double);
			break;
		}

		case Header::ROT_X:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_RotX, sizeof(double));
			WriteIdx += sizeof(double);
			break;
		}
		case Header::ROT_Y:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_RotY, sizeof(double));
			WriteIdx += sizeof(double);
			break;
		}
		case Header::ROT_Z:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_RotZ, sizeof(double));
			WriteIdx += sizeof(double);
			break;
		}

		case Header::IS_FALLING:
		{
			CopyMemory(_pOutStartPt + WriteIdx, &m_bIsJumping, sizeof(bool));
			WriteIdx += sizeof(bool);
			break;
		}

		default:
			break;
		}
	}

	fill(m_Header.begin(), m_Header.end(), false); // reset header is enough only in Read func

	return WriteIdx;
}

bool Packet::Read(char* _pInStartPt, const UINT32 _InSize)
{
	fill(m_Header.begin(), m_Header.end(), false);

	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
	{
		m_Header[i] = (bool)_pInStartPt[i];
	}

	UINT32 ReadIdx = static_cast<UINT32>(Header::MAX);
	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
	{
		if (!m_Header[i]) continue;

		switch (static_cast<Header>(i))
		{
		case Header::MSG_TYPE:
		{
			char MsgTypeVal = 0;
			CopyMemory(&MsgTypeVal, _pInStartPt + ReadIdx, sizeof(char));
			ReadIdx++;
			m_MsgType = static_cast<MsgType>(MsgTypeVal);
			break;
		}

		case Header::SESSION_IDX:
		{
			CopyMemory(&m_SessionIdx, _pInStartPt + ReadIdx, sizeof(UINT32));
			ReadIdx += sizeof(UINT32);
			break;
		}

		case Header::POS_X:
		{
			CopyMemory(&m_PosX, _pInStartPt + ReadIdx, sizeof(double));
			ReadIdx += sizeof(double);
			break;
		}
		case Header::POS_Y:
		{
			CopyMemory(&m_PosY, _pInStartPt + ReadIdx, sizeof(double));
			ReadIdx += sizeof(double);
			break;
		}
		case Header::POS_Z:
		{
			CopyMemory(&m_PosZ, _pInStartPt + ReadIdx, sizeof(double));
			ReadIdx += sizeof(double);
			break;
		}

		case Header::ROT_X:
		{
			CopyMemory(&m_RotX, _pInStartPt + ReadIdx, sizeof(double));
			ReadIdx += sizeof(double);
			break;
		}
		case Header::ROT_Y:
		{
			CopyMemory(&m_RotY, _pInStartPt + ReadIdx, sizeof(double));
			ReadIdx += sizeof(double);
			break;
		}
		case Header::ROT_Z:
		{
			CopyMemory(&m_RotZ, _pInStartPt + ReadIdx, sizeof(double));
			ReadIdx += sizeof(double);
			break;
		}

		case Header::IS_FALLING:
		{
			CopyMemory(&m_bIsJumping, _pInStartPt + ReadIdx, sizeof(bool));
			ReadIdx += sizeof(bool);
			break;
		}
		default:
			break;
		}
	}
	
	return true;
}

/*
bool Packet::CacheWrite(shared_ptr<Packet> _InPack)
{
	if (_InPack.get() == this) return false;

	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
	{
		m_Header[i] = _InPack->GetHeader()[i];
		if (!_InPack->GetHeader()[i]) continue;

		switch (static_cast<Header>(i))
		{
		case Header::MSG_TYPE:
			m_MsgType = _InPack->GetMsgType();
			break;
		case Header::SESSION_IDX:
			m_SessionIdx = _InPack->GetSessionIdx();
			break;
		case Header::POS_X:
			m_PosX = _InPack->GetPosX();
			break;
		case Header::POS_Y:
			m_PosY = _InPack->GetPosY();
			break;
		case Header::POS_Z:
			m_PosZ = _InPack->GetPosZ();
			break;
		case Header::ROT_X:
			m_RotX = _InPack->GetRotX();
			break;
		case Header::ROT_Y:
			m_RotY = _InPack->GetRotY();
			break;
		case Header::ROT_Z:
			m_RotZ = _InPack->GetRotZ();
			break;
		case Header::IS_FALLING:
			m_bIsJumping = _InPack->GetIsJumping();
			break;
		default:
			break;
		}
	}

	return true;
}
*/
