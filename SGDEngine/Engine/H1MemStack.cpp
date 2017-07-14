#include "H1EnginePrivate.h"
#include "H1MemStack.h"

using namespace SGD::Memory;

#if !FINAL_RELEASE
void H1MemStack::CreateTrackStackAlloc(byte* StartAddress, uint64 Size)
{
	// create new node
	H1TrackStackAlloc* NewNode = new H1TrackStackAlloc();
	NewNode->Next = TrackStackAllocHead;
	NewNode->StartAddress = StartAddress;
	NewNode->Size = Size;
	NewNode->MemoryTag = Head;

	// update head
	TrackStackAllocHead = NewNode;
}

void H1MemStack::ReleaseTrackStackAlloc(byte* StartAddress, uint64 Size)
{
	// check whether current head has same Size
	h1MemCheckf(TrackStackAllocHead->Size == Size, "invalid operation to release the memory stack (%d != %d)", TrackStackAllocHead->Size, Size);
	h1MemCheckf(TrackStackAllocHead->StartAddress == StartAddress, "invalid operation to release the memory stack (%x != %x)", TrackStackAllocHead->StartAddress, StartAddress);

	// release head node
	H1TrackStackAlloc* RemoveNode = TrackStackAllocHead;
	// update the node first
	TrackStackAllocHead = TrackStackAllocHead->Next;
	// delete the node
	delete RemoveNode;
}

void H1MemStack::ReleaseTackStackAllocByAddress(byte* EndAddress, MemoryTagHeader* MemoryTag)
{
	// while reaching to the MemoryTag, release TrackStackAlloc
	while (TrackStackAllocHead != nullptr && TrackStackAllocHead->MemoryTag != MemoryTag)
	{
		H1TrackStackAlloc* RemoveNode = TrackStackAllocHead;
		TrackStackAllocHead = TrackStackAllocHead->Next;
		delete RemoveNode;
	}

	h1MemCheck(TrackStackAllocHead != nullptr, "please check did you put valid memory tag!");

	// now, we are reaching to the memory tag and until reaching EndAddress, release node
	while (TrackStackAllocHead->MemoryTag == MemoryTag && TrackStackAllocHead->StartAddress != EndAddress)
	{
		H1TrackStackAlloc* RemoveNode = TrackStackAllocHead;
		TrackStackAllocHead = TrackStackAllocHead->Next;
		delete RemoveNode;
	}

	h1MemCheck(TrackStackAllocHead->MemoryTag == MemoryTag, "please check did you put valid end address!");
	h1MemCheck(TrackStackAllocHead->StartAddress == EndAddress, "please check did you put valid end address!");
}

void H1MemStack::ReleaseTackStackAllocAll()
{	
	// looping the list and release the nodes
	while (TrackStackAllocHead != nullptr)
	{
		H1TrackStackAlloc* RemoveNode = TrackStackAllocHead;
		TrackStackAllocHead = TrackStackAllocHead->Next;
		delete RemoveNode;
	}
}
#endif

H1MemStack::H1MemStack()
	: Head(nullptr)
	, MarkHead(nullptr)
	, CurrAddress(nullptr)
#if !FINAL_RELEASE
	, TrackStackAllocHead(nullptr)
#endif
{

}

H1MemStack::~H1MemStack()
{

}

// push the memory size
byte* H1MemStack::Push(uint64 Size)
{
	h1MemCheckf(Size < MemoryTag::DATA_SIZE, "Size should be smaller than MemoryTag::DATA_SIZE(%d KB)", MemoryTag::DATA_SIZE / 1024);

	byte* BaseAddress = nullptr;
	uint64 AllocatedSize = 0;
	uint64 AvailableSize = 0;

	if (Head == nullptr)
	{
		// do nothing
	}
	else
	{
		// check if the current memory tag has available size
		BaseAddress = Head->GetStartAddress();

		AllocatedSize = CurrAddress - BaseAddress;
		AvailableSize = MemoryTag::DATA_SIZE - AllocatedSize;
	}

	if (Size > AvailableSize)
	{
		if (Head != nullptr)
		{
			// mark end address
			Head->EndAddress = CurrAddress;
		}

		// allocate new memory tag
		Head = MemoryTagFactory::CreateMemoryTag(Head);

		// update the properties
		CurrAddress = Head->GetStartAddress();
	}

	// move the memory address
	byte* AllocatedAddress = CurrAddress;
	CurrAddress += Size;

#if !FINAL_RELEASE
	CreateTrackStackAlloc(AllocatedAddress, Size);
#endif

	return AllocatedAddress;
}

bool H1MemStack::Pop(uint64 Size)
{
	h1MemCheckf(Size < MemoryTag::DATA_SIZE, "Size should be smaller than MemoryTag::DATA_SIZE(%d KB)", MemoryTag::DATA_SIZE / 1024);

	// mark address should be higher than updated curr address (subtracted by size)
	h1MemCheckf(MarkHead->MarkedAddress < CurrAddress - Size, "please check the range of memory mark");

	uint64 CurrSize = Size;

	// if the current address reach to the head release the memory tag
	byte* StartAddress = Head->GetStartAddress();
	if (StartAddress >= CurrAddress - Size)
	{
		// update current address
		CurrSize -= (CurrAddress - StartAddress);

		// release the memory tag until reaching to next mem mark
		FreeMemoryTags(Head->Next);
	}	

	// finally update current address
	CurrAddress -= CurrSize;

#if !FINAL_RELEASE
	ReleaseTrackStackAlloc(CurrAddress, Size);
#endif

	return true;
}

void H1MemStack::FreeMemoryTags(MemoryTagHeader* NewHead)
{
	// looping memory tags
	while (Head != NewHead)
	{
		MemoryTagHeader* CurrHead = Head;

		// update head
		Head = CurrHead->Next;

		// restore the curr address
		CurrAddress = Head->EndAddress;

		// destroy current memory tag
		MemoryTagFactory::DestroyMemoryTag(CurrHead);
	}
}