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
		H1MemStack()
			: Head(nullptr)
		{}

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
		};

		// memory tag in memory stack
		class MemoryTag
		{
		public:
			enum
			{
				HEADER_SIZE = sizeof(MemoryTagHeader),
			};

			// memory tag layout aligned to Data in union
			struct MemoryTagLayout
			{
				// memory tag header
				MemoryTagHeader Header;

				// real data which we can use except for HEADER_SIZE
				byte Data[H1MemoryArena::MEMORY_BLOCK_SIZE - HEADER_SIZE];
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
			// firend class declaration
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
		};

		// member variables
		// 1. memory tag header (Head)
		MemoryTagHeader* Head;
		// 2. current memory tag address 
		//	- it is based on Head (MemoryTagHeader*)
		byte* CurrAddress;
	};

	// motivated from UE3
	class H1MemMark
	{
	public:
		H1MemMark(H1MemStack& Owner)
			: OwnerStack(Owner)				
		{
			// get current memory tag and address
			MarkedMemoryTag = Owner.Head;
			MarkedAddress = Owner.CurrAddress;
		}

		~H1MemMark()
		{

		}

	protected:
		H1MemStack& OwnerStack;

		// marked memory tag
		H1MemStack::MemoryTagHeader* MarkedMemoryTag;
		// marked base address from memory tag
		byte* MarkedAddress;		
	};
}
}