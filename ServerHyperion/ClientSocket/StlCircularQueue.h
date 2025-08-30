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
	template <class... P>
	StlCircularQueue(size_t buffer_size, P&&... _Params)
	{
		//assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));
		
		size_t BitCeiled = bit_ceil(buffer_size);
		buffer_mask_ = BitCeiled - 1;
		buffer_.resize(BitCeiled);

		for (size_t i = 0; i != BitCeiled; i += 1)
		{
			buffer_[i] = make_shared<T>(forward<P>(_Params)...);
			buffer_[i].sequence_.store(i, memory_order_relaxed);
		}

		enqueue_pos_.store(0, memory_order_relaxed);
		dequeue_pos_.store(0, memory_order_relaxed);
	}

	~StlCircularQueue()
	{
		// no need to delete for shared_ptr objs...
	}

	inline bool enqueue(const T& data)
	{
		shared_ptr<cell_t> pCell;
		size_t pos = enqueue_pos_.load(memory_order_relaxed);

		for (;;)
		{
			pCell = buffer_[pos & buffer_mask_];
			size_t seq = pCell->sequence_.load(memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)pos;

			if (dif == 0)		// seq == pos
			{
				/*
				* compare_exchange_weak ����...
				* ���� enqueue_pos_�� ���� pos�� ������ pos + 1�� ���� �����ϰ� true ��ȯ.
				* �ݸ� enqueue_pos_�� ���� pos�� �ٸ��� ��ٷ� false ��ȯ.
				* 
				*/
				if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, memory_order_relaxed))
					break;
			}
			else if (dif < 0)	// seq < pos -> �̹� ä���� �ְ� dequeue�� ��ٸ��� �ε����� �������� ��
				return false;
			else				// seq > pos -> �̹� ���� dequeue���� ������ ���� ����� �Ѿ�ų�
				pos = enqueue_pos_.load(memory_order_relaxed);
		}

		pCell->data_ = data;
		pCell->sequence_.store(pos + 1, memory_order_release);

		return true;
	}

	inline bool dequeue(T& data)
	{
		shared_ptr<cell_t> cell;
		size_t pos = dequeue_pos_.load(memory_order_relaxed);

		for (;;)
		{
			cell = buffer_[pos & buffer_mask_];
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

	void operator = (StlCircularQueue const&);

private:
	struct cell_t
	{
		atomic<size_t>		sequence_;
		T					data_;
	};

	vector<shared_ptr<cell_t>> buffer_;
	size_t				buffer_mask_;

	atomic<size_t>			enqueue_pos_;
	atomic<size_t>			dequeue_pos_;

	static const size_t     cacheline_size = 64;
	typedef char            cacheline_pad_t[cacheline_size];
	cacheline_pad_t         pad0_;
	cacheline_pad_t         pad1_;
	cacheline_pad_t         pad2_;
	cacheline_pad_t         pad3_;

	StlCircularQueue(const StlCircularQueue&);
};
