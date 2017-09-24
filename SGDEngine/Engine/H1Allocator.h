#pragma once

#include "H1MemoryLogger.h"

// block allocator
#include "H1BlockAllocPolicy.h"

namespace SGD
{
namespace Memory
{
	template <class AllocPolicy>
	class H1Allocator
	{
	public:
		byte* Allocate(uint64 InSize)
		{
			return Policy.Allocate(InSize);
		}

		void Deallocate(byte* InAddress)
		{
			Policy.Deallocate(InAddress);
		}

	protected:
		AllocPolicy Policy;
	};

	// default block allocator
	template <int32 BlockSize, int32 Alignment = 4>
	class H1BlockAllocatorDefault : public H1Allocator < H1BlockAllocPolicy<H1BlockAllocParams<BlockSize, Alignment> > >
	{
	};
}
}