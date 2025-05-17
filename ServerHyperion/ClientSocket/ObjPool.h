#pragma once
#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif


#include <iostream>
#include <concurrent_queue.h>
#include <utility>

using namespace std;
using namespace Concurrency;

template <typename T>
class SERVERHYPERION_API ObjPool
{
public:
	template <class... P>
	ObjPool(size_t _InInitSize, P&&... params);

	bool Acquire(shared_ptr<T> _pOutElem);
	void Return(shared_ptr<T> _pInElem);

private:
	concurrent_queue <shared_ptr< T >> m_Data;
};

template<typename T>
template<class ...P>
inline ObjPool<T>::ObjPool(size_t _InInitSize, P&&... params)
{
	for (size_t i = 0; i < 60; ++i)
	{
		m_Data.push(make_shared<T>(forward<P>(params)...));
	}
}

template<typename T>
inline bool ObjPool<T>::Acquire(shared_ptr<T> _pOutElem)
{
	return m_Data.try_pop(_pOutElem);
}

template<typename T>
inline void ObjPool<T>::Return(shared_ptr<T> _pInElem)
{
	m_Data.push(_pInElem);
}
