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
#include "CircularQueue.h"

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

private:
	CircularQueue <shared_ptr< T >> m_Data;
};

template<typename T>
template<class ...P>
inline ObjPool<T>::ObjPool(size_t _InInitSize, P&&... _Params)
	: m_Data(CircularQueue<shared_ptr<T>>(_InInitSize))
{	
	for (size_t i = 0; i < _InInitSize; ++i)
	{
		m_Data.Enqueue(make_shared<T>(forward<P>(_Params)...));
	}
}

template<typename T>
inline shared_ptr<T> ObjPool<T>::Acquire()
{
	if (m_Data.IsEmpty()) 
		return nullptr;
	
	shared_ptr<T> RetPtr;
	m_Data.Dequeue(RetPtr);
	return RetPtr;
}

template<typename T>
inline void ObjPool<T>::Return(shared_ptr<T>& _pInElem)
{
	m_Data.Enqueue(_pInElem);
}

