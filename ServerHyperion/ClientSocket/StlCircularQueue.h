// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif

#include <iostream>
#include <atomic>
#include "StlCircularBuffer.h"

using namespace std;

// WARNING: This queue is planned for deprecation in favor of TSpscQueue

/**
 * Implements a lock-free first-in first-out queue using a circular array.
 *
 * This class is thread safe only in single-producer single-consumer scenarios.
 *
 * The number of items that can be enqueued is one less than the queue's capacity,
 * because one item will be used for detecting full and empty states.
 *
 * There is some room for optimization via using fine grained memory fences, but
 * the implications for all of our target platforms need further analysis, so
 * we're using the simpler sequentially consistent model for now.
 *
 * @param T The type of elements held in the queue.
 */
typedef unsigned int		uint32;
typedef signed int	 		int32;

template<typename T> 
class StlCircularQueue
{
public:
	using FElementType = T;

	template <typename T>
	__forceinline constexpr remove_reference_t<T>&& MoveTemp(T&& Obj) noexcept
	{
		using CastType = std::remove_reference_t<T>;

		// Validate that we're not being passed an rvalue or a const object - the former is redundant, the latter is almost certainly a mistake
		static_assert(is_lvalue_reference_v<T>, "MoveTemp called on an rvalue");
		static_assert(!is_same_v<CastType&, const CastType&>, "MoveTemp called on a const object");

		return (CastType&&)Obj;
	}

	/**
	 * Constructor.
	 *
	 * @param CapacityPlusOne The number of elements that the queue can hold (will be rounded up to the next power of 2).
	 */
	template<class ...P>
	StlCircularQueue(uint32 CapacityPlusOne = 0, P&&... _Params)
		: Buffer(CapacityPlusOne, forward<P>(_Params)...)
		, Head(0)
		, Tail(0)
	{
	}

	__forceinline StlCircularQueue& operator=(const StlCircularQueue& _InCirQ)
	{
		if (this == &_InCirQ) return *this;

		Buffer = _InCirQ.Buffer;
		Head.store(_InCirQ.Head.load());
		Tail.store(_InCirQ.Tail.load());

		return *this;
	}

	inline auto begin() const { return Buffer.begin(); }
	inline auto end() const { return Buffer.end(); }

public:
	/**
	 * Gets the number of elements in the queue.
	 *
	 * Can be called from any thread. The result reflects the calling thread's current
	 * view. Since no locking is used, different threads may return different results.
	 *
	 * @return Number of queued elements.
	 */
	uint32 Count() const
	{
		int32 Count = Tail.load() - Head.load();

		if (Count < 0)
		{
			Count += Buffer.Capacity();
		}

		return (uint32)Count;
	}

	/**
	 * Removes an item from the front of the queue.
	 *
	 * @param OutElement Will contain the element if the queue is not empty.
	 * @return true if an element has been returned, false if the queue was empty.
	 * @note To be called only from consumer thread.
	 */
	bool Dequeue(FElementType& OutElement)
	{
		const uint32 CurrentHead = Head.load();

		if (CurrentHead != Tail.load())
		{
			OutElement = MoveTemp(Buffer[CurrentHead]);
			Head.store(Buffer.GetNextIndex(CurrentHead));

			return true;
		}

		return false;
	}

	/**
	 * Removes an item from the front of the queue.
	 *
	 * @return true if an element has been removed, false if the queue was empty.
	 * @note To be called only from consumer thread.
	 */
	bool Dequeue()
	{
		const uint32 CurrentHead = Head.load();

		if (CurrentHead != Tail.load())
		{
			Head.store(Buffer.GetNextIndex(CurrentHead));

			return true;
		}

		return false;
	}

	/**
	 * Empties the queue.
	 *
	 * @note To be called only from consumer thread.
	 * @see IsEmpty
	 */
	void Empty()
	{
		Head.store(Tail.load());
	}

	/**
	 * Adds an item to the end of the queue.
	 *
	 * @param Element The element to add.
	 * @return true if the item was added, false if the queue was full.
	 * @note To be called only from producer thread.
	 */
	bool Enqueue(const FElementType& Element)
	{
		const uint32 CurrentTail = Tail.load();
		uint32 NewTail = Buffer.GetNextIndex(CurrentTail);

		if (NewTail != Head.load())
		{
			Buffer[CurrentTail] = Element;
			Tail.store(NewTail);

			return true;
		}

		return false;
	}

	/**
	 * Adds an item to the end of the queue.
	 *
	 * @param Element The element to add.
	 * @return true if the item was added, false if the queue was full.
	 * @note To be called only from producer thread.
	 */
	bool Enqueue(FElementType&& Element)
	{
		const uint32 CurrentTail = Tail.load();
		uint32 NewTail = Buffer.GetNextIndex(CurrentTail);

		if (NewTail != Head.load())
		{
			Buffer[CurrentTail] = MoveTemp(Element);
			Tail.store(NewTail);

			return true;
		}

		return false;
	}

	/**
	 * Checks whether the queue is empty.
	 *
	 * Can be called from any thread. The result reflects the calling thread's current
	 * view. Since no locking is used, different threads may return different results.
	 *
	 * @return true if the queue is empty, false otherwise.
	 * @see Empty, IsFull
	 */
	__forceinline bool IsEmpty() const
	{
		return (Head.load() == Tail.load());
	}

	/**
	 * Checks whether the queue is full.
	 *
	 * Can be called from any thread. The result reflects the calling thread's current
	 * view. Since no locking is used, different threads may return different results.
	 *
	 * @return true if the queue is full, false otherwise.
	 * @see IsEmpty
	 */
	bool IsFull() const
	{
		return (Buffer.GetNextIndex(Tail.load()) == Head.load());
	}

	/**
	 * Returns the oldest item in the queue without removing it.
	 *
	 * @param OutItem Will contain the item if the queue is not empty.
	 * @return true if an item has been returned, false if the queue was empty.
	 * @note To be called only from consumer thread.
	 */
	bool Peek(FElementType& OutItem) const
	{
		const uint32 CurrentHead = Head.load();

		if (CurrentHead != Tail.load())
		{
			OutItem = Buffer[CurrentHead];

			return true;
		}

		return false;
	}

	/**
	 * Returns the oldest item in the queue without removing it.
	 *
	 * @return an FElementType pointer if an item has been returned, nullptr if the queue was empty.
	 * @note To be called only from consumer thread.
	 * @note The return value is only valid until Dequeue, Empty, or the destructor has been called.
	 */
	const FElementType* Peek() const
	{
		const uint32 CurrentHead = Head.load();

		if (CurrentHead != Tail.load())
		{
			return &Buffer[CurrentHead];
		}

		return nullptr;
	}

private:
	/** Holds the buffer. */
	StlCircularBuffer<FElementType> Buffer;

	/** Holds the index to the first item in the buffer. */
	atomic<uint32> Head;

	/** Holds the index to the last item in the buffer. */
	atomic<uint32> Tail;
};
