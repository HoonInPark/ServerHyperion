#pragma once

#include <string>
#include <fstream>
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

	bool ReadFile(unsigned char** _pInData, int* _InLen);
	int WriteToFile();

	void Sample(char* _pInChar, size_t _InSize);

private:
	bool ModByteArr(char* _pInChar, size_t _InSize);

private:
	string m_FilePath;

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

inline bool PacketSampler::ReadFile(unsigned char** _pInData, int* _InLen)
{
	ifstream Is(m_FilePath, ifstream::binary);
	if (Is)
	{
		// seekg를 이용한 파일 크기 추출
		Is.seekg(0, Is.end);
		int Len = (int)Is.tellg();
		Is.seekg(0, Is.beg);

		// malloc으로 메모리 할당
		unsigned char* pBuf = (unsigned char*)malloc(Len);

		// read data as a block:
		Is.read((char*)pBuf, Len);
		Is.close();
		*_pInData = pBuf;
		*_InLen = Len;
	}

	return true;
}

inline int PacketSampler::WriteToFile()
{
	char* pData;
	
	if (m_Data.size() != m_Meta.size()) return 1;

	streamsize TotalLen = 0;
	for (const auto PackLen : m_Meta)
		TotalLen += PackLen;

	pData = new char[TotalLen];
	size_t CurBinIdx = 0;
	for (size_t i = 0; i < m_Meta.size(); ++i)
	{
		for (size_t j = 0; j < m_Meta[i]; ++j)
		{
			pData[CurBinIdx] = m_Data[i][j];
			CurBinIdx++;
		}
	}

	ofstream Fout;
	Fout.open(m_FilePath, ios::out | ios::binary);

	if (Fout.is_open())
	{
		Fout.write((const char*)pData, TotalLen);
		Fout.close();
	}

	delete[] pData;

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
