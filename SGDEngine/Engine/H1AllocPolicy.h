#pragma once

// memory log
#include "H1MemoryLogger.h"

// for default alloc policy
#include "H1MemoryArena.h"

// for chunk allocation, it needs to be syncchonized
#include "H1CriticalSection.h"

// for preventing ABA problem
#include "H1TaggedPointer.h"

namespace SGD
{
namespace Memory
{
	// abstract class for forcing the layout of alloc page
	class H1AllocPagePolicy
	{
	public:
		// forcing to define its own alloc page for different alloc page policy
		class H1AllocPage {};

		H1AllocPage* Allocate()
		{
			h1MemCheck(false, "H1AllocPagePolicy should not be used itself");
			return nullptr;
		}

		void Deallocate(H1AllocPage* InAllocPage)
		{
			h1MemCheck(false, "H1AllocPagePolicy should not be used itself");
		}
	};

	/*
		- default alloc page policy using MemoryArena
		- 64KB page allocation
		- 2MB chunk allocation
	*/
	class H1DefaultAllocPagePolicy : public H1AllocPagePolicy
	{
	public:
		H1DefaultAllocPagePolicy()
			: H1AllocPagePolicy()
			, FreeHead(nullptr)
		{

		}

		// H1AllocPage definition
		class H1AllocPage
		{
		public:
			class H1AllocPageHeader
			{
			public:
				H1AllocPageHeader()
					: Next(nullptr)
				{}

				// next page
				H1AllocPage* Next;
			};

			enum
			{
				HeaderSize = sizeof(H1AllocPageHeader),
				DataSize = (63 * 1024) - HeaderSize,	// 64KB page allocation (excluding HeaderSize)
				PageSize = HeaderSize + DataSize,
			};			

			H1AllocPage() {}

			// public methods
			H1AllocPage* GetNext() { return Header.Next; }
			void SetNext(H1AllocPage* InNext) { Header.Next = InNext; }

			int32 GetSize() { return DataSize; }
			byte* GetData() { return (byte*)this + HeaderSize; }

		protected:
			friend class H1DefaultAllocPagePolicy;

			H1AllocPageHeader Header;
		};

		H1AllocPage* Allocate() 
		{
			SGD::Thread::H1TaggedPointer<H1AllocPage> NewHead;
			SGD::Thread::H1TaggedPointer<H1AllocPage> OldHead;

			do 
			{
				OldHead = FreeHead;

				if (OldHead.GetNode() == nullptr)
				{
					CreateNewChunk();

					// restart from setting Result!
					continue;
				}

				// construct new head
				NewHead = OldHead;
				NewHead.SetNode(OldHead.GetNode()->GetNext());
				NewHead.IncrementTag();

			} while (OldHead.GetNode() != nullptr
				&& (SGD::Thread::H1TaggedPointer<H1AllocPage>)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(FreeHead), (int64)NewHead, (int64)OldHead) != OldHead);

			return OldHead.GetNode();
		}

		void Deallocate(H1AllocPage* InAllocPage)
		{
			SGD::Thread::H1TaggedPointer<H1AllocPage> NewHead;
			SGD::Thread::H1TaggedPointer<H1AllocPage> OldHead;

			do
			{
				OldHead = FreeHead;

				NewHead = OldHead;
				InAllocPage->SetNext(OldHead.GetNode());
				NewHead.SetNode(InAllocPage);
				NewHead.IncrementTag();

			} while ((SGD::Thread::H1TaggedPointer<H1AllocPage>)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(FreeHead), (int64)NewHead, (int64)OldHead) != OldHead);
		}

	protected:
		// managing page (MT supported)
		SGD::Thread::H1TaggedPointer<H1AllocPage> FreeHead;

	protected:
		// using memory arena, manage the chunk which gives number of pages
		class H1AllocChunk
		{
		public:
			static H1AllocChunk* CreateChunk()
			{
				return new H1AllocChunk();
			}

			int32 GetSize() { return MemoryBlock.Size; }
			byte* GetStartAddress() { return MemoryBlock.BaseAddress; }

			// next alloc chunk
			H1AllocChunk* Next;

			// destruction is working as usual
			~H1AllocChunk()
			{
				H1GlobalSingleton::MemoryArena()->DeallocateMemoryBlock(MemoryBlock);
			}

		protected:
			friend class H1DefaultAllocPagePolicy;

			H1AllocChunk()
				: MemoryBlock(H1GlobalSingleton::MemoryArena()->AllocateMemoryBlock())
				, Next(nullptr)
			{

			}		

		private:
			SGD::Memory::H1MemoryBlock MemoryBlock;
		};

		void CreateNewChunk()
		{
			H1AllocChunk* NewChunk = H1AllocChunk::CreateChunk();

			// link new chunk in lock-free
			SGD::Thread::H1TaggedPointer<H1AllocChunk> NewChunkHead;
			SGD::Thread::H1TaggedPointer<H1AllocChunk> OldChunkHead;
			
			do 
			{
				OldChunkHead = ChunkHead;
				NewChunk->Next = OldChunkHead.GetNode()->Next;

				NewChunkHead = OldChunkHead;
				NewChunkHead.SetNode(NewChunk);
				NewChunkHead.IncrementTag();

			} while ((SGD::Thread::H1TaggedPointer<H1AllocChunk>)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&ChunkHead, (int64)NewChunkHead, (int64)OldChunkHead) != OldChunkHead);

			// generate free pages based on ChunkHead
			H1AllocChunk* NewHead = NewChunk;
			int32 PageCount = NewHead->GetSize() / H1AllocPage::PageSize;			
			byte* CurrAddress = NewHead->GetStartAddress();

			H1AllocPage* NewFreePages = nullptr;
			H1AllocPage* FreePageTail = (H1AllocPage*)(CurrAddress);

			for (int32 Index = 0; Index < PageCount; ++Index)
			{
				H1AllocPage* CurrPage = (H1AllocPage*)(CurrAddress);				

				// link the pages
				CurrPage->SetNext(NewFreePages);
				NewFreePages = CurrPage;

				CurrAddress += H1AllocPage::PageSize;
			}

			// link to the free list (in lock-free way)
			SGD::Thread::H1TaggedPointer<H1AllocPage> NewPageHead;
			SGD::Thread::H1TaggedPointer<H1AllocPage> OldPageHead;

			do 
			{
				OldPageHead = FreeHead;
				FreePageTail->SetNext(OldPageHead.GetNode());

				NewPageHead = OldPageHead;
				NewPageHead.SetNode(NewFreePages);
				NewPageHead.IncrementTag();
			} while ((SGD::Thread::H1TaggedPointer<H1AllocPage>)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&FreeHead, (int64)NewPageHead, (int64)OldPageHead) != OldPageHead);
		}

		// managing chunk
		SGD::Thread::H1TaggedPointer<H1AllocChunk> ChunkHead;
	};

	// base class for alloc policy
	template <class AllocPagePolicyType = H1DefaultAllocPagePolicy>
	class H1AllocPolicy 
	{
	public:
		// page definition for alloc policy
		typedef typename AllocPagePolicyType AllocPagePolicy;

		// page definition
		typedef typename AllocPagePolicy::H1AllocPage AllocPage;

		// necessary methods to override to use allocator
		void* Allocate(uint64 InSize)
		{
			h1MemCheck(false, "H1AllocPolicy should be not created!");
			return nullptr;
		}
		void Deallocate(void* InPointer)
		{
			h1MemCheck(false, "H1AllocPolicy should be not created!");
		}
	};
}
}