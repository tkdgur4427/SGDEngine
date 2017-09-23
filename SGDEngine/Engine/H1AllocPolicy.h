#pragma once

// memory log
#include "H1MemoryLogger.h"

// for default alloc policy
#include "H1MemoryArena.h"

// for chunk allocation, it needs to be syncchonized
#include "H1CriticalSection.h"

namespace SGD
{
namespace Memory
{
	// abstract class for forcing the layout of alloc page
	class H1AllocPagePolicy
	{
	public:
		// forcing to define its own alloc page for different alloc page policy
		class H1AllocPage;

		virtual H1AllocPage* Allocate() = 0;
		virtual void Deallocate(const H1AllocPage* InAllocPage) = 0;
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

		virtual H1AllocPage* Allocate() final
		{
			H1AllocPage* Result = nullptr;
			H1AllocPage* NewHead = nullptr;

			do 
			{
				Result = FreeHead;

				if (Result == nullptr)
				{
					CreateNewChunk();

					// restart from setting Result!
					continue;
				}

				NewHead = Result->GetNext();

			} while (Result != nullptr
				&& (H1AllocPage*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(FreeHead), (int64)NewHead, (int64)Result) != Result);
		}

		virtual void Deallocate(const H1AllocPage* InAllocPage) final
		{
			H1AllocPage* NewHead = InAllocPage;
			H1AllocPage* OldHead = nullptr;

			do
			{
				OldHead = FreeHead;
				NewHead->SetNext(OldHead);
			} while ((H1AllocPage*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(FreeHead), (int64)NewHead, (int64)OldHead) != OldHead);
		}

	protected:
		// managing page (MT supported)
		H1AllocPage* FreeHead;

	protected:
		// using memory arena, manage the chunk which gives number of pages
		class H1AllocChunk
		{
		public:
			static SGD::unique_ptr<H1AllocChunk> CreateChunk()
			{
				return SGD::make_unique<H1AllocChunk>();
			}

			int32 GetSize() { return MemoryBlock.Size; }
			byte* GetStartAddress() { return MemoryBlock.BaseAddress; }

			// next alloc chunk
			SGD::unique_ptr<H1AllocChunk> Next;

		protected:
			H1AllocChunk()
				: MemoryBlock(H1GlobalSingleton::MemoryArena()->AllocateMemoryBlock())
			{

			}

			~H1AllocChunk()
			{
				H1GlobalSingleton::MemoryArena()->DeallocateMemoryBlock(MemoryBlock);
			}			

		private:
			SGD::Memory::H1MemoryBlock MemoryBlock;
		};

		void CreateNewChunk()
		{
			SGD::unique_ptr<H1AllocChunk> NewChunk = H1AllocChunk::CreateChunk();

			NewChunk->Next = SGD::move(ChunkHead);
			ChunkHead = SGD::move(NewChunk);

			// generate free pages based on ChunkHead
			H1AllocChunk* NewHead = ChunkHead.get();
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
			H1AllocPage* CurrHead = nullptr;
			do 
			{
				CurrHead = FreeHead;
				FreePageTail->SetNext(CurrHead);
			} while ((H1AllocPage*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&FreeHead, (int64)NewFreePages, (int64)CurrHead) != CurrHead);
		}

		// managing chunk
		SGD::unique_ptr<H1AllocChunk> ChunkHead;
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
		virtual void* Allocate(uint64 InSize) = 0;
		virtual void Deallocate(void* InPointer) = 0;
	};
}
}