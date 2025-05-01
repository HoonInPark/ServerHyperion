#pragma once

#define WIN32_LEAN_AND_MEAN
#define TO_BE_DEPRECATED 1
#define IS_IN_RECV 1
#include <windows.h>

class PacketData
{
#if TO_BE_DEPRECATED
public:
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	char* pPacketData = nullptr;

	void Set(PacketData& value)
	{
		SessionIndex = value.SessionIndex;
		DataSize = value.DataSize;

		pPacketData = new char[value.DataSize];
		CopyMemory(pPacketData, value.pPacketData, value.DataSize);
	}

	void Set(UINT32 sessionIndex_, UINT32 dataSize_, char* pData)
	{
		SessionIndex = sessionIndex_;
		DataSize = dataSize_;

		pPacketData = new char[dataSize_];
		CopyMemory(pPacketData, pData, dataSize_);
	}

	void Release()
	{
		delete pPacketData;
	}
#endif

	/*
	* <header structure>
	* filedname used as enum and idx : char (1 byte)
	* isEmpty						 : bool	(1 byte)
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
	PacketData() = default;

	struct Data
	{
		int SessionId{ 0 };

		double PosX{ 0.f };
		double PosY{ 0.f };
		double PosZ{ 0.f };
		double RotX{ 0.f };
		double RotY{ 0.f };
		double RotZ{ 0.f };

		bool IsJump{ false };
	};

	// For object pooling, you shouldn't make the conversion happen only in the constructor, 
	// but rather modify it so that it can happen after creation.
	bool Compress(const Data& _InData);
	bool Decompress(const void* _pInStartPt, const size_t _InSize);

private:
};
