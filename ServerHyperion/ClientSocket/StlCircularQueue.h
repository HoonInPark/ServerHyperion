#pragma once

#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif

#include <cassert>
#include <atomic>
#include <vector>
#include <bit>

using namespace std;

template<typename T>
class StlCircularQueue
{
public:
	template<class ...P>
	StlCircularQueue(size_t buffer_size, P&&... _Params)
		: buffer_(buffer_size, cell_t(forward<P>(_Params)...))
		, buffer_mask_(buffer_size - 1)
	{
		for (size_t i = 0; i != buffer_size; i += 1)
			buffer_[i].sequence_.store(i, memory_order_relaxed);

		enqueue_pos_.store(0, memory_order_relaxed);
		dequeue_pos_.store(0, memory_order_relaxed);
	}

	~StlCircularQueue()
	{
	}

	bool enqueue(T const& data)
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
				/*
				* compare_exchange_weak 설명...
				* 만약 enqueue_pos_의 값이 pos와 같으면 pos + 1로 값을 변경하고 true 반환.
				* 반면 enqueue_pos_의 값이 pos와 다르면 곧바로 false 반환.
				* 
				*/
				if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, memory_order_relaxed))
					break;
			}
			else if (dif < 0)	// seq < pos -> 이미 채워져 있고 dequeue를 기다리는 인덱스에 접근했을 때
				return false;
			else				// seq > pos -> 이미 누가 dequeue까지 끝나서 다음 라운드로 넘어갔거나
				pos = enqueue_pos_.load(memory_order_relaxed);
		}

		cell->data_ = data;
		cell->sequence_.store(pos + 1, memory_order_release);

		return true;
	}

	bool dequeue(T& data)
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

		data = cell->data_;
		cell->sequence_.store(pos + buffer_mask_ + 1, memory_order_release);

		return true;
	}

private:
	struct cell_t
	{
		cell_t() = default;

		template<class ...P>
		explicit cell_t(P&&... args)
			: data_(std::forward<P>(args)...)
		{
		}

		atomic<size_t>		sequence_;
		T					data_;
	};

	vector<cell_t>          buffer_;
	atomic<size_t>			enqueue_pos_;
	atomic<size_t>			dequeue_pos_;

	static const size_t     cacheline_size = 64;
	typedef char            cacheline_pad_t[cacheline_size];
	cacheline_pad_t         pad0_;
	const size_t            buffer_mask_;
	cacheline_pad_t         pad1_;
	cacheline_pad_t         pad2_;
	cacheline_pad_t         pad3_;

	StlCircularQueue(StlCircularQueue const&);
	void operator = (StlCircularQueue const&);
};