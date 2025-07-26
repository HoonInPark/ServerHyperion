#pragma once

#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif

#include <iostream>
#include <deque>
#include <utility>

using namespace std;

template <typename T>
class ObjPool
{
public:
	ObjPool() = default;

	template <class... P>
	ObjPool(size_t _InInitSize, P&&... params);

	shared_ptr<T> Acquire();
	void Return(shared_ptr<T>& _pInElem);

	inline auto begin() const { return m_Data.begin(); }
	inline auto end() const { return m_Data.end(); }

private:
	size_t m_SizeLim;
	deque <shared_ptr< T >> m_Data;
};

template<typename T>
template<class ...P>
inline ObjPool<T>::ObjPool(size_t _InInitSize, P&&... params)
	: m_SizeLim(_InInitSize)
{
	for (size_t i = 0; i < _InInitSize; ++i)
	{
		m_Data.push_back(make_shared<T>(forward<P>(params)...));
	}
}

template<typename T>
inline shared_ptr<T> ObjPool<T>::Acquire()
{
	if (m_Data.empty()) 
		return nullptr;
	
	shared_ptr<T> RetPtr = m_Data.front();
	m_Data.pop_front();

	return RetPtr;
}

template<typename T>
inline void ObjPool<T>::Return(shared_ptr<T>& _pInElem)
{
	if (m_SizeLim > m_Data.size())
		m_Data.push_back(_pInElem);
	else
		printf("ObjPool Error : You're trying to push more than its initial size!!!");
}

