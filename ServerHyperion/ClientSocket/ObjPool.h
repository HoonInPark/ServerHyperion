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
	template <class... P>
	ObjPool(size_t _InInitSize, P&&... _Params)
		: m_Data(_InInitSize, forward<P>(_Params)...)
	{
	}

	bool Acquire(shared_ptr<T>& _OutRetPtr);
	bool Return(const shared_ptr<T>& _pInElem);

private:
	StlCircularQueue<T> m_Data;
};

template<typename T>
inline bool ObjPool<T>::Acquire(shared_ptr<T>& _OutRetPtr)
{	
	return m_Data.dequeue(_OutRetPtr);
}

template<typename T>
inline bool ObjPool<T>::Return(const shared_ptr<T>& _pInElem)
{
	return m_Data.enqueue(_pInElem);
}

