#include "H1EnginePrivate.h"
#include "H1MemoryArena.h"
using namespace SGD::Memory;

// for using bit manipulation
#include "H1PlatformUtil.h"
using namespace SGD::Platform::Util;

// only for memory use formatted debugf and checkf
#if !FINAL_RELEASE

// memory debugf
// debugf with message
#define h1MemDebug(message) H1Log<char>::CreateLog(message);
// debugf with formatted message
#define h1MemDebugf(format, ...) H1Log<char>::CreateFormattedLog(format, __VA_ARGS__);
// memory checkf
// checkf with message
#define h1MemCheck(condition, meessage) if(!condition) { H1Log<char>::CreateLog(meessage); H1Log<char>::ForceToDump(); __debugbreak(); }
// checkf with formatted message
#define h1MemCheckf(condition, format, ...) if(!condition) { H1Log<char>::CreateFormattedLog(format, __VA_ARGS__); H1Log<char>::ForceToDump(); __debugbreak(); }

#else

// disable debugf and checkf for final release mode
#define h1MemDebug(message) __noop
#define h1MemDebugf(format, ...) __noop
#define h1MemCheck(condition, meessage) __noop
#define h1MemCheckf(condition, format, ...) __noop

#endif

H1MemoryArena::MemoryPage::AllocOutput H1MemoryArena::MemoryPage::Allocate(const AllocInput& Params)
{
	AllocOutput Output;
	Output.TagId = Layout.TagId;
	Output.Offset = -1;
	Output.Count = 0;

	// get the available block index
	int32 Offset = GetAvailableBlockIndex(Params.BlockCount);
	if (Offset != -1)
	{
		// mark alloc bit
		MarkAllocBits(true, Offset, Params.BlockCount);

		// update output results
		Output.Offset = Offset;
		Output.Count = Params.BlockCount;
	}

	return Output;
}

void H1MemoryArena::MemoryPage::Deallocate(const DeallocInput& Params)
{
#if !FINAL_RELEASE
	// validation check
	ValidateAllocBits(true, Params.Offset, Params.Count);
#endif

	// just mark as free
	MarkAllocBits(false, Params.Offset, Params.Count);
}

int32 H1MemoryArena::MemoryPage::GetAvailableBlockIndex(int32 InBlockCount)
{
	int32 Offset = -1;

	// copy the bit alloc flag
	uint64 AllocMask = Layout.AllocBitMask;
	// copy the block count
	int32 BlockCount = InBlockCount;

	// if the block count becomes zero, it found the offset
	uint32 StartOffset = -1;
	uint32 CurrOffset = 0;
	while (BlockCount > 0)
	{
		// first find the nonzero bit		
		if (!appBitScanForward64(CurrOffset, AllocMask))
		{
			// just break the loop and return it
			break;
		}

		if (CurrOffset + BlockCount > MEMORY_BLOCK_COUNT) // if it reach to the end, break the loop
		{
			break;
		}

		// update start offset
		StartOffset = CurrOffset;

		// test and set it as allocated (as bit 1)		
		while (BlockCount > 0)
		{
			// test bit and set it as allocated
			if (appBitTestAndSet64(CurrOffset, AllocMask))
			{
				CurrOffset++;
				BlockCount--;
			}
			else
			{
				// reset the block count and stop looping
				BlockCount = InBlockCount;
				break;
			}
		}
	}

	// if the block count reach to 0, update the offset
	if (BlockCount == 0)
	{
		Offset = StartOffset;
	}

	return Offset;
}

void H1MemoryArena::MemoryPage::MarkAllocBits(bool InValue, int32 InOffset, int32 InCount)
{
	// InValue is same, so we don't need to worry about inner branch prediction (for performance issue)
	for (int32 CurrOffset = InOffset; CurrOffset < InCount; ++CurrOffset)
	{
		if (InValue) // mark it as allocated
		{
			Layout.AllocBitMask |= (1 << CurrOffset);
		}
		else // mark it as free (deallocated)
		{
			Layout.AllocBitMask &= ~(1 << CurrOffset);
		}
	}
}

void H1MemoryArena::MemoryPage::ValidateAllocBits(bool InValue, int32 InOffset, int32 InCount)
{
	for (int32 CurrOffset = InOffset; CurrOffset < InCount; ++CurrOffset)
	{
		if (InValue) // mark it as allocated
		{
			// trigger assert
			h1MemCheck((Layout.AllocBitMask & (1 << CurrOffset)) == 0, "invalid alloc bit please check!");		
		}
		else // mark it as free (deallocated)
		{
			// trigger assert
			h1MemCheck((Layout.AllocBitMask ^ (1 << CurrOffset)) == 1, "invalid alloc bit please check!");
		}
	}
}
