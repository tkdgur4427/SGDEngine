#pragma once

#include "H1GlobalSingleton.h"
#include "H1Memory.h"
#include "H1RingBuffer.h"

namespace SGD
{
namespace Log
{
	template <typename CharType>
	struct H1LogNode
	{
		// single linked list node
		H1LogNode* Next;

		// mark node size
		int32 NodeSize;

		// string length
		int32 StrLen;

		// real string data (start pointer)
		CharType Data[0];

		// real string data will be inserted (NodeSize - sizeof(H1LogNode) = StrLen)
	};

	// dedicated log ring buffer
	template <typename CharType>
	struct H1LogRingBuffer
	{
		H1LogRingBuffer()
			, Head(nullptr)
		{
			// request memory block from H1MemoryArena
			MemoryBlock = H1GlobalSingleton::MemoryArena()->AllocateMemoryBlock();

			// initialize ring buffer layout
			Layout.Initialize(MemoryBlock.BaseAddress, MemoryBlock.Size);
		}

		~H1LogRingBuffer()
		{
			if (MemoryBlock.BaseAddress == nullptr)
			{
				// destroy the ring buffer layout first
				Layout.Destroy();

				// deallocate memory blocks
				H1GlobalSingleton::MemoryArena()->DeallocateMemoryBlock(MemoryBlock);
			}
		}

		// memory block from MemoryArena
		H1MemoryBlock MemoryBlock;

		// node head
		H1LogNode<CharType>* Head;

		// ring buffer layout
		H1RingBufferLayout Layout;
	};

	/*
		H1AsnycLogger
			- this is logger to leave message asynchronously
			- multi-threaded logging supported
	*/

	class H1AsyncLogger
	{
	public:
	};
}
}