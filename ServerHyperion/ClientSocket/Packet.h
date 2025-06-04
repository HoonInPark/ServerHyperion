#pragma once

#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN

#define TO_BE_DEPRECATED

#include <windows.h>
#include <iostream>
#include <vector>

using namespace std;

enum class MsgType;

class SERVERHYPERION_API Packet
{
	/*
	* <header structure>
	* filedname used as enum and idx : char (1 byte)
	* sizeByteOfFieldValue			 : char	(1 byte)
	*
	* <value structure>
	*/

	/*
	* <value to send and recv>
	* pos
	* rot
	* isJump
	*/

public:
	static UINT32 GetMaxPackByteSize();

public:
	Packet();
	~Packet();

	static enum class Header : char
	{
		MSG_TYPE = 0, // CAUTIOIN: its idx must be 0 'cause of in other class it assumes that the first byte is MsgType

		SESSION_IDX,

		POS_X,
		POS_Y,
		POS_Z,

		ROT_X,
		ROT_Y,
		ROT_Z,

		IS_FALLING,

		MAX
	};

	Packet& operator=(const Packet& Other)
	{
		if (this == &Other)
			return *this;  // 자기 자신 보호

		// 헤더 복사
		m_Header = Other.m_Header;

		// 값 복사
		m_SessionIdx = Other.m_SessionIdx;

		m_PosX = Other.m_PosX;
		m_PosY = Other.m_PosY;
		m_PosZ = Other.m_PosZ;

		m_RotX = Other.m_RotX;
		m_RotY = Other.m_RotY;
		m_RotZ = Other.m_RotZ;

		m_bIsJumping = Other.m_bIsJumping;

		/*
		m_BinDataSizeTmp = Other.m_BinDataSizeTmp;

		if (Other.m_pBinData && Other.m_BinDataSizeTmp > 0)
		{
			memcpy(m_pBinData, Other.m_pBinData, m_BinDataSizeTmp);
		}
		*/

		return *this;
	}

	inline void SetMsgType(MsgType _InMsgType)
	{
		m_MsgType = _InMsgType;
		m_Header[static_cast<int>(Header::MSG_TYPE)] = true;
	}

	inline void SetSessionIdx(UINT32 _InSessionIdx)
	{
		m_SessionIdx = _InSessionIdx;
		m_Header[static_cast<int>(Header::SESSION_IDX)] = true;
	}

	inline void SetPosX(double _InPosX)
	{
		m_PosX = _InPosX;
		m_Header[static_cast<int>(Header::POS_X)] = true;
	}
	inline void SetPosY(double _InPosY)
	{
		m_PosY = _InPosY;
		m_Header[static_cast<int>(Header::POS_Y)] = true;
	}
	inline void SetPosZ(double _InPosZ)
	{
		m_PosZ = _InPosZ;
		m_Header[static_cast<int>(Header::POS_Z)] = true;
	}

	inline void SetRotPitch(double _InRotX)
	{
		m_RotX = _InRotX;
		m_Header[static_cast<int>(Header::ROT_X)] = true;
	}
	inline void SetRotRoll(double _InRotY)
	{
		m_RotY = _InRotY;
		m_Header[static_cast<int>(Header::ROT_Y)] = true;
	}
	inline void SetRotYaw(double _InRotZ)
	{
		m_RotZ = _InRotZ;
		m_Header[static_cast<int>(Header::ROT_Z)] = true;
	}

	inline void SetIsJumping(bool _InIsJumping)
	{
		m_bIsJumping = _InIsJumping;
		m_Header[static_cast<int>(Header::IS_FALLING)] = true;
	}

	// For object pooling, you shouldn't make the conversion happen only in the constructor, 
	// but rather modify it so that it can happen after creation.

	// called after Setting each value
	UINT32 Write(char*& _pOutStartPt);
	bool Read(char* _pInStartPt, const UINT32 _InSize);

	//bool CacheWrite(shared_ptr<Packet> _InPack);

	inline const vector<bool>& GetHeader() { return m_Header; }

	inline MsgType GetMsgType() const { return m_MsgType; }
	inline UINT32 GetSessionIdx() const { return m_SessionIdx; }
	inline double GetPosX() const { return m_PosX; }
	inline double GetPosY() const { return m_PosY; }
	inline double GetPosZ() const { return m_PosZ; }
	inline double GetRotX() const { return m_RotX; }
	inline double GetRotY() const { return m_RotY; }
	inline double GetRotZ() const { return m_RotZ; }
	inline bool GetIsJumping() const { return m_bIsJumping; }

	inline UINT32 GetSize() { return m_BinDataSizeTmp; }

	static inline UINT32 // return byte
		GetSize(Header _InHeaderIdx)
	{
		switch (_InHeaderIdx)
		{
		case Header::MSG_TYPE:
			return sizeof(char);

		case Header::SESSION_IDX:
			return sizeof(UINT32);

		case Header::POS_X:
			return sizeof(double);
		case Header::POS_Y:
			return sizeof(double);
		case Header::POS_Z:
			return sizeof(double);

		case Header::ROT_X:
			return sizeof(double);
		case Header::ROT_Y:
			return sizeof(double);
		case Header::ROT_Z:
			return sizeof(double);

		case Header::IS_FALLING:
			return sizeof(bool);

		default:
			return 0;
		}
	}

private:
	vector<bool> m_Header{ vector<bool>(static_cast<int>(Header::MAX), false) };

	MsgType m_MsgType;
	UINT32 m_SessionIdx{ 0 };

	double m_PosX{ 0.f };
	double m_PosY{ 0.f };
	double m_PosZ{ 0.f };

	double m_RotX{ 0.f };
	double m_RotY{ 0.f };
	double m_RotZ{ 0.f };

	bool m_bIsJumping{ false };

	////////////////////// bin //////////////////////
	char* m_pBinData;
	UINT32 m_BinDataSizeTmp;
	//////////////////// bin end ////////////////////

	/////////////////////// cache data end ///////////////////////
};
