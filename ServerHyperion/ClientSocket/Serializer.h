#pragma once

#define CLIENTSOCK_EXPORT

#ifdef CLIENTSOCK_EXPORT
#define CLIENTSOCK_API __declspec(dllexport)
#else
#define CLIENTSOCK_API __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN
#define TO_BE_DEPRECATED

#include <windows.h>
#include <vector>

using namespace std;

class CLIENTSOCK_API Serializer
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
	Serializer(bool _bIsSender);

	enum class Header : char
	{
		SESSION_IDX = 0,

		POS_X,
		POS_Y,
		POS_Z,

		ROT_X,
		ROT_Y,
		ROT_Z,

		IS_JUMPING,

		MAX
	};

	inline void SetSessionIdx(UINT32 _InSessionIdx)
	{
		m_SessionIdx = _InSessionIdx;
		m_Header[static_cast<int>(Header::SESSION_IDX)] = true;
	}

	inline void SetPosX(double _InPosX)
	{
		if (m_PosX == _InPosX) return;
		m_PosX = _InPosX;
		m_Header[static_cast<int>(Header::POS_X)] = true;
	}
	inline void SetPosY(double _InPosY)
	{
		if (m_PosY == _InPosY) return;
		m_PosY = _InPosY;
		m_Header[static_cast<int>(Header::POS_Y)] = true;
	}
	inline void SetPosZ(double _InPosZ)
	{
		if (m_PosZ == _InPosZ) return;
		m_PosZ = _InPosZ;
		m_Header[static_cast<int>(Header::POS_Z)] = true;
	}

	inline void SetRotX(double _InRotX)
	{
		if (m_RotX == _InRotX) return;
		m_RotX = _InRotX;
		m_Header[static_cast<int>(Header::ROT_X)] = true;
	}
	inline void SetRotY(double _InRotY)
	{
		if (m_RotY == _InRotY) return;
		m_RotY = _InRotY;
		m_Header[static_cast<int>(Header::ROT_Y)] = true;
	}
	inline void SetRotZ(double _InRotZ)
	{
		if (m_RotZ == _InRotZ) return;
		m_RotZ = _InRotZ;
		m_Header[static_cast<int>(Header::ROT_Z)] = true;
	}

	inline void SetIsJumping(bool _InIsJumping)
	{
		if (m_IsJumping == _InIsJumping) return;
		m_IsJumping = _InIsJumping;
		m_Header[static_cast<int>(Header::IS_JUMPING)] = true;
	}

	// For object pooling, you shouldn't make the conversion happen only in the constructor, 
	// but rather modify it so that it can happen after creation.

	// called after Setting each value
	size_t Write(char* _pOutStartPt);
	bool Read(char* _pInStartPt, const size_t _InSize);

	inline UINT32 GetSessionIdx() const { return m_SessionIdx; }
	inline double GetPosX() const { return m_PosX; }
	inline double GetPosY() const { return m_PosY; }
	inline double GetPosZ() const { return m_PosZ; }
	inline double GetRotX() const { return m_RotX; }
	inline double GetRotY() const { return m_RotY; }
	inline double GetRotZ() const { return m_RotZ; }
	inline bool GetIsJumping() const { return m_IsJumping; }

private:
	__forceinline char // return byte
		GetSize(Header _InHeaderIdx)
	{
		switch (_InHeaderIdx)
		{
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

		case Header::IS_JUMPING:
			return sizeof(bool);

		default:
			return 0;
		}
	}

private:
	vector<bool> m_Header;

	///////////////////////// cache data /////////////////////////
	UINT32 m_SessionIdx{ 0 };

	double m_PosX{ 0.f };
	double m_PosY{ 0.f };
	double m_PosZ{ 0.f };

	double m_RotX{ 0.f };
	double m_RotY{ 0.f };
	double m_RotZ{ 0.f };

	bool m_IsJumping{ false };

	////////////////////// bin //////////////////////
	char* m_pBinData;
	size_t m_BinDataSizeTmp;
	//////////////////// bin end ////////////////////

	/////////////////////// cache data end ///////////////////////
};
