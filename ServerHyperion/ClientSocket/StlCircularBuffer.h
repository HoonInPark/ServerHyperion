// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#define SERVERHYPERION_EXPORT

#ifdef SERVERHYPERION_EXPORT
#define SERVERHYPERION_API __declspec(dllexport)
#else
#define SERVERHYPERION_API __declspec(dllimport)
#endif

/**
 * Template for circular buffers.
 *
 * The size of the buffer is rounded up to the next power of two in order speed up indexing
 * operations using a simple bit mask instead of the commonly used modulus operator that may
 * be slow on some platforms.
 */
#include <iostream>
#include <vector>
#include <bit>
#include <windows.h>
#include <utility>

using namespace std;

typedef unsigned int		uint32;

template<typename InElementType> 
class StlCircularBuffer
{
public:
	using ElementType = InElementType;

	/**
	 * Creates and initializes a new instance of the TCircularBuffer class.
	 *
	 * @param Capacity The number of elements that the buffer can store (will be rounded up to the next power of 2).
	 */
	StlCircularBuffer(uint32 Capacity = 0)
	{
		if (Capacity <= 0) return;
		if (Capacity > 0xffffffffU) return;

		Elements.resize(bit_ceil(Capacity));
		fill(Elements.begin(), Elements.end(), ElementType()); // TypeName{} makes it with its default constructor

		IndexMask = Elements.size() - 1;
	}

	/**
	 * Creates and initializes a new instance of the TCircularBuffer class.
	 *
	 * @param Capacity The number of elements that the buffer can store (will be rounded up to the next power of 2).
	 * @param InitialValue The initial value for the buffer's elements.
	 */
	template<class ...P>
	StlCircularBuffer(uint32 Capacity = 0, P&&... _Params) 
	{
		if (Capacity > 0xffffffffU) return;

		Elements.resize(bit_ceil(Capacity));
		fill(Elements.begin(), Elements.end(), ElementType(forward<P>(_Params)...));

		IndexMask = Elements.size() - 1;
	}

	__forceinline StlCircularBuffer& operator=(const StlCircularBuffer& _InCirBuf)
	{
		if (this == &_InCirBuf) return *this;

		Elements = _InCirBuf.Elements;
		IndexMask = _InCirBuf.IndexMask;
		return *this;
	}

public:
	/**
	 * Returns the mutable element at the specified index.
	 *
	 * @param Index The index of the element to return.
	 */
	__forceinline ElementType& operator[](uint32 Index)
	{
		return Elements[Index & IndexMask];
	}

	/**
	 * Returns the immutable element at the specified index.
	 *
	 * @param Index The index of the element to return.
	 */
	__forceinline const ElementType& operator[](uint32 Index) const
	{
		return Elements[Index & IndexMask];
	}

public:
	/**
	 * Returns the number of elements that the buffer can hold.
	 *
	 * @return Buffer capacity.
	 */
	__forceinline uint32 Capacity() const
	{
		return Elements.size();
	}

	/**
	 * Calculates the index that follows the given index.
	 *
	 * @param CurrentIndex The current index.
	 * @return The next index.
	 */
	__forceinline uint32 GetNextIndex(uint32 CurrentIndex) const
	{
		return ((CurrentIndex + 1) & IndexMask);
	}

	/**
	 * Calculates the index previous to the given index.
	 *
	 * @param CurrentIndex The current index.
	 * @return The previous index.
	 */
	__forceinline uint32 GetPreviousIndex(uint32 CurrentIndex) const
	{
		return ((CurrentIndex - 1) & IndexMask);
	}

private:
	/** Holds the mask for indexing the buffer's elements. */
	uint32 IndexMask;

	/** Holds the buffer's elements. */
	vector<ElementType> Elements;
};
