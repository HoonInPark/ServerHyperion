#include "Packet.h"

Packet::Packet()
{
	UINT32 DataSize = 0;
	DataSize += m_Header.size(); // Header size
	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
		DataSize += GetSize(static_cast<Header>(i)); // add size of each value

	m_pBinData = new char[DataSize];
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

	// write value
	for (int i = 0; i < static_cast<int>(Header::MAX); ++i)
	{
		if (!m_Header[i]) continue;

		switch (static_cast<Header>(i))
		{
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
			CopyMemory(_pOutStartPt + WriteIdx, &m_IsJumping, sizeof(bool));
			WriteIdx += sizeof(bool);
			break;
		}

		default:
			break;
		}
	}

	fill(m_Header.begin(), m_Header.end(), false);
	return WriteIdx;
}

bool Packet::Read(char* _pInStartPt, const UINT32 _InSize)
{
	_pInStartPt = m_pBinData;

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
			CopyMemory(&m_IsJumping, _pInStartPt + ReadIdx, sizeof(bool));
			ReadIdx += sizeof(bool);
			break;
		}
		default:
			break;
		}
	}

	fill(m_Header.begin(), m_Header.end(), false);
	
	return true;
}