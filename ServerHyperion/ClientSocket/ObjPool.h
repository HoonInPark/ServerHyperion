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
#include "StlCircularQueue.h"

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
	StlCircularQueue <shared_ptr< T >> m_Data;
};

template<typename T>
template<class ...P>
inline ObjPool<T>::ObjPool(size_t _InInitSize, P&&... _Params)
	: m_Data(StlCircularQueue<shared_ptr<T>>(_InInitSize))
{	
	for (size_t i = 0; i < _InInitSize; ++i)
	{
		m_Data.Enqueue(make_shared<T>(forward<P>(_Params)...));
	}
}

template<typename T>
inline shared_ptr<T> ObjPool<T>::Acquire()
{	
	shared_ptr<T> RetPtr;
	if (m_Data.Dequeue(RetPtr))
		return RetPtr;
	else
		return nullptr;
}

template<typename T>
inline void ObjPool<T>::Return(shared_ptr<T>& _pInElem)
{
	m_Data.Enqueue(_pInElem);
}

