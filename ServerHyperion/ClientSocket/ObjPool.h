#pragma once
/*
#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif
*/
#include <iostream>
#include <queue>
#include <utility>

using namespace std;

template <typename T>
class /*SERVERHYPERION_API*/ ObjPool
{
public:
	ObjPool() = default;

	template <class... P>
	ObjPool(size_t _InInitSize, P&&... params);

	shared_ptr<T> Acquire();
	void Return(shared_ptr<T>& _pInElem);

private:
	queue <shared_ptr< T >> m_Data;
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
inline shared_ptr<T> ObjPool<T>::Acquire()
{
	if (m_Data.empty()) return nullptr;
	
	shared_ptr<T> RetPtr = m_Data.front();
	m_Data.pop();

	return RetPtr;
}

template<typename T>
inline void ObjPool<T>::Return(shared_ptr<T>& _pInElem)
{
	m_Data.push(_pInElem);
}
