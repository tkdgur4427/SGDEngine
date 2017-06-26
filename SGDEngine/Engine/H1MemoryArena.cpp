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
#define h1MemCheck(condition, meessage) if(!(condition)) { H1Log<char>::CreateLog(meessage); H1Log<char>::ForceToDump(); __debugbreak(); }
// checkf with formatted message
#define h1MemCheckf(condition, format, ...) if(!(condition)) { H1Log<char>::CreateFormattedLog(format, __VA_ARGS__); H1Log<char>::ForceToDump(); __debugbreak(); }

#else

// disable debugf and checkf for final release mode
#define h1MemDebug(message) __noop
#define h1MemDebugf(format, ...) __noop
#define h1MemCheck(condition, meessage) __noop
#define h1MemCheckf(condition, format, ...) __noop

#endif

H1MemoryArena::MemoryPage::AllocOutput H1MemoryArena::AllocateInternal(const MemoryPage::AllocInput& Input)
{
	// alloc output
	MemoryPage::AllocOutput Output;

	// looping current page, and try to allocate page
	MemoryPage* CurrPage = FreePageHead;
	while (CurrPage != nullptr)
	{
		Output = CurrPage->Allocate(Input);
		if (Output.Offset != -1)
		{
			// successfully allocate block
			break;
		}

		// move to next page
		CurrPage = CurrPage->GetNextPage();
	}

	// check whether the block is allocated
	if (Output.Offset == -1)
	{
		// allocate new page
		MemoryPage* NewPage = AllocatePage();

		// allocate new output
		Output = NewPage->Allocate(Input);
		h1MemCheck(Output.Offset != -1, "Error! please check this");
	}

	return Output;
}

void H1MemoryArena::DeallocateInternal(const MemoryPage::DeallocInput& Input)
{
	// looping naive memory pages
	MemoryPage* CurrPage = PageHead.get();
	while (CurrPage != nullptr)
	{
		// check whether the current page match tag id
		if (CurrPage->Layout.TagId == Input.TagId)
		{
			CurrPage->Deallocate(Input);
			break;
		}

		// move to next page
		CurrPage = CurrPage->GetNextPage();
	}

	h1MemCheck(CurrPage != nullptr, "failed to find memory page, please check!");

	// looping free memory page and if curr page is not available, insert it
	MemoryPage* CurrFreePage = FreePageHead;
	while (CurrFreePage != nullptr)
	{
		if (CurrFreePage->Layout.TagId == CurrPage->Layout.TagId)
		{
			// it is already exists in free page
			break;
		}

		// move to next free page
		CurrFreePage = CurrFreePage->GetNextFreePage();
	}

	// add curr free page list (link it properly)
	if (CurrFreePage == nullptr)
	{
		CurrPage->SetNextFreePage(FreePageHead);
		FreePageHead = CurrPage;
	}
}

H1MemoryBlock H1MemoryArena::AllocateMemoryBlock()
{
	// create the alloc input
	MemoryPage::AllocInput Input;
	Input.BlockCount = 1;

	// alloc output
	MemoryPage::AllocOutput Output = AllocateInternal(Input);

	// successfully create memory block
	H1MemoryBlock NewBlock(Output.TagId, Output.Offset);
	NewBlock.BaseAddress = Output.BaseAddress;
	NewBlock.Size = H1MemoryArena::MEMORY_BLOCK_SIZE;

	return NewBlock;
}

H1MemoryBlockRange H1MemoryArena::AllocateMemoryBlocks(int32 MemoryBlockCount)
{
	// create the alloc input
	MemoryPage::AllocInput Input;
	Input.BlockCount = MemoryBlockCount;

	// alloc output
	MemoryPage::AllocOutput Output = AllocateInternal(Input);

	// successfully create memory block
	H1MemoryBlockRange NewBlockRange(Output.TagId, Output.Offset, Output.Count);
	NewBlockRange.BaseAddress = Output.BaseAddress;
	NewBlockRange.Size = H1MemoryArena::MEMORY_BLOCK_SIZE * Output.Count;

	return NewBlockRange;
}

void H1MemoryArena::DeallocateMemoryBlock(const H1MemoryBlock& InMemoryBlock)
{
	// create the dealloc input
	MemoryPage::DeallocInput Input;
	Input.TagId = InMemoryBlock.PageTagId;
	Input.Offset = InMemoryBlock.Offset;
	Input.Count = 1;

	// deallocate it
	DeallocateInternal(Input);
}

void H1MemoryArena::DeallocateMemoryBlocks(const H1MemoryBlockRange& InMemoryBlocks)
{
	// create the dealloc input
	MemoryPage::DeallocInput Input;
	Input.TagId = InMemoryBlocks.PageTagId;
	Input.Offset = InMemoryBlocks.Offset;
	Input.Count = InMemoryBlocks.Count;

	// deallocate it
	DeallocateInternal(Input);
}

H1MemoryArena::MemoryPage* H1MemoryArena::AllocatePage()
{
	// create new page
	eastl::unique_ptr<MemoryPage> NewPage = eastl::make_unique<MemoryPage>();
	// reset the page
	SGD::Platform::Util::appMemzero((byte*)NewPage.get(), sizeof(MemoryPage));

	// properly link new page
	NewPage->SetNextPage(PageHead);
	PageHead = eastl::move(NewPage);

	// properly link new free page
	NewPage->SetNextFreePage(FreePageHead);
	FreePageHead = eastl::move(NewPage.get());

	return NewPage.get();
}

void H1MemoryArena::DeallocateAllPages()
{
	// looping all free page node
	while (FreePageHead != nullptr)
	{
		// move to next free page (implicitly invalidate free page pointer)
		FreePageHead = FreePageHead->Layout.NextFreePage;
	}

	// looping all pages
	while (PageHead.get() == nullptr)
	{
		// get the unique
		eastl::unique_ptr<MemoryPage> TempPageHead = eastl::move(PageHead);

		// move next page
		PageHead = eastl::move(TempPageHead.get()->Layout.NextPage);

		// release the previous page head
		TempPageHead.reset();
	}
}

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

	// set base address
	Output.BaseAddress = (byte*)&Layout.MemoryBlocks[Output.Offset];

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
