#pragma once

#include <string>
#include <fstream>
#include <windows.h>
#include <vector>

using namespace std;

/// <summary>
/// it samples byte data from server while it is running 
/// sample byte data is saved and gonna be used with dummy client
/// </summary>
class PacketSampler
{
public:
	PacketSampler();
	~PacketSampler();

	bool ReadFile();
	int WriteToFile();

	void Sample(char* _pInChar, size_t _InSize);

private:
	bool ModByteArr(char* _pInChar, size_t _InSize);

private:
	string m_DataFilePath{ string() };
	string m_MetaFilePath{ string() };

	vector<char*>  m_Data;
	vector<size_t> m_Meta;

};

PacketSampler::PacketSampler()
{
}

PacketSampler::~PacketSampler()
{
	for (auto pData : m_Data)
		delete[] pData;

}

inline bool PacketSampler::ReadFile()
{
	////////////////////////////////////////////////////////////////////////////////
	// read meta data
	////////////////////////////////////////////////////////////////////////////////
	ifstream ifs(m_MetaFilePath, ios::binary);
	size_t count;
	ifs.read(reinterpret_cast<char*>(&count), sizeof(count));
	ifs.read(reinterpret_cast<char*>(m_Meta.data()), count * sizeof(size_t));
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////

	ifstream Is(m_DataFilePath, ifstream::binary);
	if (Is)
	{
		// seekg를 이용한 파일 크기 추출
		Is.seekg(0, Is.end);
		int Len = (int)Is.tellg();
		Is.seekg(0, Is.beg);

		// malloc으로 메모리 할당
		char* pData = (char*)malloc(Len);

		// read data as a block:
		Is.read((char*)pData, Len);
		Is.close();

		m_Data.reserve(m_Meta.size());
		// do something with pData
		size_t CurDataFileIdx = 0;
		for (size_t i = 0; i < m_Meta.size(); ++i)
		{
			char* pElem = new char[m_Meta[i]];
			CopyMemory(pElem, pData + CurDataFileIdx, m_Meta[i]);

			m_Data.push_back(pElem);

			CurDataFileIdx += m_Meta[i];
		}
	}

	return true;
}

inline int PacketSampler::WriteToFile()
{
	if (m_DataFilePath.empty() || m_MetaFilePath.empty()) return 1;
	if (m_Data.size() != m_Meta.size()) return 2;

	streamsize TotalLen = 0;
	for (const auto PackLen : m_Meta)
		TotalLen += PackLen;

	char* pData = new char[TotalLen];
	size_t CurDataFileIdx = 0;
	for (size_t i = 0; i < m_Meta.size(); ++i)
	{
		for (size_t j = 0; j < m_Meta[i]; ++j)
		{
			pData[CurDataFileIdx] = m_Data[i][j];
			CurDataFileIdx++;
		}
	}
	
	ofstream Fout;
	Fout.open(m_DataFilePath, ios::out | ios::binary);

	if (Fout.is_open())
	{
		Fout.write((const char*)pData, TotalLen);
		Fout.close();
	}

	delete[] pData;

	////////////////////////////////////////////////////////////////////////////////
	// writh meta data
	////////////////////////////////////////////////////////////////////////////////
	std::ofstream ofs(m_MetaFilePath, ios::binary);
	size_t count = m_Meta.size();
	ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));
	ofs.write(reinterpret_cast<const char*>(m_Meta.data()), count * sizeof(size_t));
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////

	return 0;
}

inline void PacketSampler::Sample(char* _pInChar, size_t _InSize)
{
	char* pSampleBuf = new char[_InSize];

	for (size_t i = 0; i < _InSize; ++i)
		pSampleBuf[i] = _pInChar[i];	
	
	ModByteArr(pSampleBuf, _InSize);

	m_Data.push_back(pSampleBuf);
	m_Meta.push_back(_InSize);
}

inline bool PacketSampler::ModByteArr(char* _pInChar, size_t _InSize)
{
	// do something


	return true;
}
