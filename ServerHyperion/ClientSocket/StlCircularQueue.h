#pragma once

#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif

#include <iostream>
#include <cassert>
#include <atomic>
#include <vector>
#include <bit>

using namespace std;

template<typename T>
class StlCircularQueue
{
public:
	StlCircularQueue(size_t _InBufSize)
		: buffer_mask_(bit_ceil(_InBufSize) - 1)
	{
		size_t BitBufSize = bit_ceil(_InBufSize);
		buffer_ = new cell_t[BitBufSize];

		for (size_t i = 0; i != BitBufSize; i += 1)
			buffer_[i].sequence_.store(i, memory_order_relaxed);

		enqueue_pos_.store(0, memory_order_relaxed);
		dequeue_pos_.store(0, memory_order_relaxed);
	}

	~StlCircularQueue()
	{
		delete[] buffer_;
	}

	bool enqueue(unique_ptr<T>& data)
	{
		cell_t* cell;
		size_t pos = enqueue_pos_.load(memory_order_relaxed);

		for (;;)
		{
			cell = &buffer_[pos & buffer_mask_];
			size_t seq = cell->sequence_.load(memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)pos;

			if (dif == 0)		// seq == pos
			{
				if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, memory_order_relaxed))
					break;
			}
			else if (dif < 0)
				return false;
			else
				pos = enqueue_pos_.load(memory_order_relaxed);
		}

		cell->data_ = move(data);
		cell->sequence_.store(pos + 1, memory_order_release);

		return true;
	}

	bool dequeue(unique_ptr<T>& data)
	{
		cell_t* cell;
		size_t pos = dequeue_pos_.load(memory_order_relaxed);

		for (;;)
		{
			cell = &buffer_[pos & buffer_mask_];
			size_t seq = cell->sequence_.load(memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

			if (dif == 0)
			{
				if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, memory_order_relaxed))
					break;
			}
			else if (dif < 0)
				return false;
			else
				pos = dequeue_pos_.load(memory_order_relaxed);
		}

		data = move(cell->data_);
		cell->data_ = nullptr;
		cell->sequence_.store(pos + buffer_mask_ + 1, memory_order_release);

		return true;
	}

private:
	struct cell_t
	{
		atomic<size_t>		sequence_;
		unique_ptr<T>		data_;
	};

	//vector<cell_t>			buffer_;
	cell_t* buffer_;

	const size_t            buffer_mask_;

	atomic<size_t>			enqueue_pos_;
	atomic<size_t>			dequeue_pos_;

	static const size_t     cacheline_size = 64;
	typedef char            cacheline_pad_t[cacheline_size];
	cacheline_pad_t         pad0_;
	cacheline_pad_t         pad1_;
	cacheline_pad_t         pad2_;
	cacheline_pad_t         pad3_;

	StlCircularQueue(StlCircularQueue const&);
	void operator = (StlCircularQueue const&);
};

template<typename T>
class StlObjectPool
{
public:
	StlObjectPool(size_t _InBufSize);
	~StlObjectPool();

	inline bool enqueue(unique_ptr<T>& _pInData) { return m_Data->enqueue(_pInData); }
	inline bool dequeue(unique_ptr<T>& _pInData) { return m_Data->dequeue(_pInData); }

private:
	StlCircularQueue<T>* m_Data;
};

template<typename T>
StlObjectPool<T>::StlObjectPool(size_t _InBufSize)
{
	m_Data = new StlCircularQueue<T>(_InBufSize);
	for (size_t i = 0; i < _InBufSize; ++i)
	{
		auto pElem = make_unique<T>();
		m_Data->enqueue(pElem);
	}
}

template<typename T>
StlObjectPool<T>::~StlObjectPool()
{
	delete m_Data;
}
