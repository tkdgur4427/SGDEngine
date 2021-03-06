#pragma once

// memory log
#include "H1MemoryLogger.h"

// for default alloc policy
#include "H1MemoryArena.h"

// for chunk allocation, it needs to be syncchonized
#include "H1CriticalSection.h"

// for preventing ABA problem
#include "H1LockFreeStackImpl.h"

namespace SGD
{
namespace Memory
{
	// abstract class for forcing the layout of alloc page
	class H1AllocPagePolicy
	{
	public:
		H1AllocPagePolicy() {}
		virtual ~H1AllocPagePolicy() {}

		// forcing to define its own alloc page for different alloc page policy
		class H1AllocPage 
		{
		public:
			int64 GetSize() { return -1; }
			byte* GetData() { return nullptr; }
		};

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
		{

		}

		virtual ~H1DefaultAllocPagePolicy()
		{
			DestroyAllChunks();
		}

		// H1AllocPage definition
		class H1AllocPage : public SGD::Container::SinglelyLinkedList::H1Node
		{
		public:
			enum
			{
				HeaderSize = sizeof(SGD::Container::SinglelyLinkedList::H1Node),
				DataSize = (64 * 1024) - HeaderSize,	// 64KB page allocation (excluding HeaderSize)
				PageSize = HeaderSize + DataSize,
			};			

			H1AllocPage() {}
			virtual ~H1AllocPage() {}

			// public methods
			H1AllocPage* GetNext() { return Next; }
			void SetNext(H1AllocPage* InNext) { Next = InNext; }

			int64 GetSize() { return (int64)DataSize; }
			byte* GetData() { return (byte*)this + HeaderSize; }

		protected:
			friend class H1DefaultAllocPagePolicy;
		};

		H1AllocPage* Allocate() 
		{
			// if there is no valid node exists, create new chunk!
			if (FreeHead.GetNode() == nullptr)
			{
				CreateNewChunk();
			}

			// pop the new page from the free head
			SGD::Container::SinglelyLinkedList::H1Node* NewNode = SGD::Thread::LockFreeStack::Pop(FreeHead);
			H1AllocPage* NewPage = static_cast<H1AllocPage*>(NewNode);

			return NewPage;
		}

		void Deallocate(H1AllocPage* InAllocPage)
		{
			SGD::Thread::LockFreeStack::Push(FreeHead, InAllocPage);
		}

	protected:
		// managing page (MT supported)
		SGD::Thread::LockFreeStack::H1LfsHead FreeHead;

		// using memory arena, manage the chunk which gives number of pages
		class H1AllocChunk : public SGD::Container::SinglelyLinkedList::H1Node
		{
		public:
			static H1AllocChunk* CreateChunk()
			{
				return new H1AllocChunk();
			}

			int32 GetSize() { return MemoryBlock.Size; }
			byte* GetStartAddress() { return MemoryBlock.BaseAddress; }

			// destruction is working as usual
			virtual ~H1AllocChunk()
			{
				H1GlobalSingleton::MemoryArena()->DeallocateMemoryBlock(MemoryBlock);
			}

		protected:
			friend class H1DefaultAllocPagePolicy;

			H1AllocChunk()
				: SGD::Container::SinglelyLinkedList::H1Node()
				, MemoryBlock(H1GlobalSingleton::MemoryArena()->AllocateMemoryBlock())
			{

			}		

		private:
			SGD::Memory::H1MemoryBlock MemoryBlock;
		};

		void CreateNewChunk()
		{
			H1AllocChunk* NewChunk = H1AllocChunk::CreateChunk();

			// link new chunk in lock-free
			SGD::Thread::LockFreeStack::Push(ChunkHead, NewChunk);

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
			SGD::Thread::LockFreeStack::Push(FreeHead, NewFreePages, FreePageTail);
		}

		void DestroyAllChunks()
		{
			// this method should be called in synchronized env.
			H1AllocChunk* CurrChunk = (H1AllocChunk*)ChunkHead.GetNode();
			while (CurrChunk != nullptr)
			{
				H1AllocChunk* ChunkToRemove = CurrChunk;
				CurrChunk = (H1AllocChunk*)CurrChunk->Next;
				delete ChunkToRemove;
			}
		}

		// managing chunk
		SGD::Thread::LockFreeStack::H1LfsHead ChunkHead;
	};

	/*
		- default only one large page alloc support
		- only allowed to allocate once! (user-specified size)
	*/
	class H1DefaultAllocOneLargePagePolicy : public H1AllocPagePolicy
	{
	public:
		enum
		{
			// to allocate large one memory page, calculate required block count for MemoryArena
			MEM_BLOCK_SIZE = 2 * 1024 * 1024,
		};

		H1DefaultAllocOneLargePagePolicy(uint64 InSize)
			: H1AllocPagePolicy()
			, Size(InSize)
		{

		}

		virtual ~H1DefaultAllocOneLargePagePolicy()
		{

		}

		class H1AllocPage
		{
		public:
			H1AllocPage(uint64 InSize)
				: LargeMemBlock(H1GlobalSingleton::MemoryArena()->AllocateMemoryBlocks((InSize + (MEM_BLOCK_SIZE - 1)) / MEM_BLOCK_SIZE))
			{

			}

			~H1AllocPage()
			{
				H1GlobalSingleton::MemoryArena()->DeallocateMemoryBlocks(LargeMemBlock);
			}

			int64 GetSize() { return LargeMemBlock.Size; }
			byte* GetData() { return LargeMemBlock.BaseAddress; }

		protected:
			SGD::Memory::H1MemoryBlockRange LargeMemBlock;
		};

		H1AllocPage* Allocate()
		{
			SGD::Thread::H1ScopeLock Lock(&SyncObject);

			h1Check(LargeAllocPage != nullptr, "already large memory page is allocated!");
			LargeAllocPage =  SGD::make_unique<H1AllocPage>(Size);
			return LargeAllocPage.get();
		}

		void Deallocate(H1AllocPage* InAllocPage)
		{
			h1Check(false, "deallocate method is not supported!");
		}

	protected:
		// only one large page is supported, so do the lock
		SGD::Thread::H1CriticalSection SyncObject;

		SGD::unqiue_ptr<H1AllocPage> LargeAllocPage;
		uint64 Size;
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