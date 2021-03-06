#pragma once

#include "H1MemoryArena.h"
#include "H1GlobalSingleton.h"

namespace SGD
{
namespace Memory
{
	/*
		H1MemoryStack
			- motivated from UE3 (MemoryStack)
	*/
	class H1MemStack
	{
	public:
		H1MemStack();
		~H1MemStack();

		byte* Push(uint64 Size);
		bool Pop(uint64 Size);
		bool Pop(void* Pointer);

		void Tick()
		{
			//MarkHead == nullptr
		}

	protected:
		// friend class declaration
		friend class H1MemMark;

		// inner forward declaration
		class MemoryTag;

		// memory tag header
		struct MemoryTagHeader
		{		
			// memory block data (real placeholder for MemoryTag)
			H1MemoryBlock MemoryBlock;

			// memory tag indicator
			MemoryTag* MemoryTag;

			// next memory tag 
			MemoryTagHeader* Next;

			// lastly allocate pointer
			//	- this address is NOT same as MemoryBlock.BaseAddress + MemoryTag::DATA_SIZE
			byte* EndAddress;

			// public methods
			byte* GetStartAddress() { return &MemoryTag->Data[0]; }
		};

		// memory tag in memory stack
		class MemoryTag
		{
		public:
			enum
			{
				HEADER_SIZE = sizeof(MemoryTagHeader),
				DATA_SIZE = H1MemoryArena::MEMORY_BLOCK_SIZE - HEADER_SIZE,
			};

			// memory tag layout aligned to Data in union
			struct MemoryTagLayout
			{
				// memory tag header
				MemoryTagHeader Header;

				// real data which we can use except for HEADER_SIZE
				byte Data[DATA_SIZE];
			};			
			
			union
			{
				// memory tag layout
				MemoryTagLayout Layout;

				// raw data
				//	- exactly same as H1MemoryArena::MemoryBlock				
				byte Data[H1MemoryArena::MEMORY_BLOCK_SIZE];
			};			

		protected:
			// friend class declaration
			friend class H1MemoryTagFactory;

			// memory tag can not create manually (MemoryTag should be mapped into H1MemoryBlock)
			MemoryTag()
			{
				// the memory tag layout should be same as memory block size
				SGD_CT_ASSERT(sizeof(MemoryTagLayout) == H1MemoryArena::MEMORY_BLOCK_SIZE);
			}			
		};

		// memory tag factory
		//	- memory tag is only created from memory tag factory
		class MemoryTagFactory
		{
		public:
			static MemoryTagHeader* CreateMemoryTag(MemoryTagHeader* Next = nullptr)
			{
				// create memory block
				H1MemoryBlock NewMemoryBlock = H1GlobalSingleton::MemoryArena()->AllocateMemoryBlock();

				// map memory tag to the new memory block
				MemoryTag* Tag = (MemoryTag*)NewMemoryBlock.BaseAddress;

				// update the value
				Tag->Layout.Header.MemoryBlock = NewMemoryBlock;
				Tag->Layout.Header.MemoryTag = Tag;
				
				if (Next == nullptr)
				{
					Tag->Layout.Header.Next = nullptr;
				}
				else
				{					
					// newly created memory tag will be linked to the Head
					Tag->Layout.Header.Next = Next;
				}

				return &(Tag->Layout.Header);
			}

			static void DestroyMemoryTag(MemoryTagHeader* Header)
			{
				// destroy memory block
				H1MemoryBlock MemoryBlock = Header->MemoryBlock;
				H1GlobalSingleton::MemoryArena()->DeallocateMemoryBlock(MemoryBlock);
			}
		};

		// free memory tags
		//	- reaching to NewHead
		void FreeMemoryTags(MemoryTagHeader* NewHead);

		// member variables
		// 1. memory tag header (Head)
		MemoryTagHeader* Head;
		// 2. current memory tag address 
		//	- it is based on Head (MemoryTagHeader*)
		byte* CurrAddress;
		// 3. memory marks
		H1MemMark* MarkHead;

#if !FINAL_RELEASE
		// tracking mem stack alloc
		struct H1TrackStackAlloc
		{
			byte* StartAddress;
			MemoryTagHeader* MemoryTag;

			uint64 Size;
			H1TrackStackAlloc* Next;
		};

		H1TrackStackAlloc* TrackStackAllocHead;

		// methods
		void CreateTrackStackAlloc(byte* StartAddress, uint64 Size);
		void ReleaseTrackStackAlloc(byte* StartAddress, uint64 Size);
		void ReleaseCurrentHeadTrackStackAllocInternal();

		void ReleaseTackStackAllocByAddress(byte* EndAddress, MemoryTagHeader* MemoryTag);
		void ReleaseTackStackAllocAll();
#endif
	};

	// motivated from UE3
	class H1MemMark
	{
	public:
		H1MemMark(H1MemStack& Owner)
			: OwnerStack(Owner)				
		{
			// get current memory tag and address
			MarkedMemoryTag = OwnerStack.Head;
			MarkedAddress = OwnerStack.CurrAddress;

			// link the memory mark
			Next = OwnerStack.MarkHead;
			OwnerStack.MarkHead = this;
		}

		~H1MemMark()
		{
			// pop the mark
			Pop();

			// unlink the memory mark
			OwnerStack.MarkHead = Next;
		}			

	protected:
		// friend class declaration
		friend class H1MemStack;

		// pop the memory stack reaching to the memmark
		void Pop()
		{
			// free memory tags
			OwnerStack.FreeMemoryTags(MarkedMemoryTag);

			// update curr address
			OwnerStack.CurrAddress = MarkedAddress;

#if !FINAL_RELEASE
			// remove debugging information for memory stack tracer
			OwnerStack.ReleaseTackStackAllocByAddress(MarkedAddress, MarkedMemoryTag);
#endif
		}

		H1MemStack& OwnerStack;

		// marked memory tag
		H1MemStack::MemoryTagHeader* MarkedMemoryTag;
		// marked base address from memory tag
		byte* MarkedAddress;		
		// linked list memory mark
		H1MemMark* Next;
	};
}
}

// operator new overrides
inline void* operator new(std::size_t Size, SGD::Memory::H1MemStack& MemStack)
{
	return MemStack.Push(Size);
}

inline void operator delete(void* Pointer, SGD::Memory::H1MemStack& MemStack)
{
	// do nothing
}

inline void* operator new[](std::size_t Size, SGD::Memory::H1MemStack& MemStack)
{
	return MemStack.Push(Size);
}

inline void operator delete[](void* Pointer, SGD::Memory::H1MemStack& MemStack)
{
	// do nothing
}