#pragma once

#include "H1AllocPolicy.h"

#include "H1LockFreeStackImpl.h"

namespace SGD
{
namespace Memory
{
	/*
		Block allocator policy
			- policy for block allocator
	*/

	// compile-time block alloc parameter definitions
	template <int32 InDataSize, int32 InAlignment = 4>
	class H1BlockAllocParams
	{
	public:
		enum 
		{
			// block alignment
			BlockAlignment = InAlignment,
			// block size
			BlockDataSize = SGD::Platform::Util::Align(InDataSize, BlockAlignment),		
		};

		H1BlockAllocParams()
		{
			h1MemCheck((BlockAlignment & ~(1 << 1)) == 0, "block alignment size should be power of two");
		}
	};

	template <class BlockAllocParam, class AllockPagePolicy = H1DefaultAllocPagePolicy>
	class H1BlockAllocPolicy : public BlockAllocParam, public H1AllocPolicy<AllockPagePolicy>
	{
	public:
		class H1AllocBlock
		{
		public:
			struct H1AllocBlockHeader : public SGD::Container::SinglelyLinkedList::H1Node
			{
				enum
				{
					BlockHeaderSize = SGD::Platform::Util::Align(sizeof(SGD::Container::SinglelyLinkedList::H1Node), BlockAllocParam::BlockAlignment),
					BlockSize = BlockAllocParam::BlockDataSize + BlockHeaderSize,
				};

				// get the real data pointer
				byte* GetData() { return (byte*)this + BlockHeaderSize; }

				// helper method for restoring block header
				static H1AllocBlockHeader* RestoreAllocBlockHeader(byte* InData)
				{
					byte* RestoredHeader = InData - BlockHeaderSize;
					return (H1AllocBlockHeader*)RestoreHeader;
				}
			};

			struct BlockLayout
			{
				H1AllocBlockHeader Header;
				byte Data[BlockAllocParam::BlockDataSize];
			};

			union 
			{
				BlockLayout Layout;
				byte Data[H1AllocBlockHeader::BlockSize];
			};
		};

		H1BlockAllocPolicy()
			: FreeBlockHead(nullptr)
			, PageHead(nullptr)
		{
			Initialize();
		}

		~H1BlockAllocPolicy()
		{
			Destroy();
		}

		byte* Allocate(uint64 InSize) 
		{
			h1MemCheck(InSize == BlockAllocParam::BlockDataSize, "size should be same as block data size!");

			// if the head is nullptr, create new page
			if (FreeBlockHead.GetNode() == nullptr)
			{
				CreateNewPage();
			}

			// pop new block
			SGD::Container::SinglelyLinkedList::H1Node* NewNode = SGD::Thread::LockFreeStack::Pop(FreeBlockHead);
			H1AllocBlock::H1AllocBlockHeader* NewBlockHeader = (H1AllocBlock::H1AllocBlockHeader*)NewNode;

			return NewBlockHeader->GetData();
		}

		void Deallocate(byte* InPointer) 
		{
			H1AllocBlock::H1AllocBlockHeader* Header = H1AllocBlock::H1AllocBlockHeader::RestoreAllocBlockHeader(InPointer);
			
			// push to the free block head
			SGD::Thread::Push(FreeBlockHead, Header);
		}

	protected:
		void Initialize()
		{

		}

		void Destroy()
		{
			FreeBlockHead = nullptr;

			// deallocate all pages
			AllocPage* CurrPage = PageHead;
			while (CurrPage != nullptr)
			{
				AllocPage* PageToRemove = CurrPage;
				CurrPage = CurrPage->GetNext();

				// deallocate the page
				PageToRemove->SetNext(nullptr);
				PagePolicy.Deallocate(PageToRemove);
			}
		}

		void CreateNewPage()
		{
			// allocate new page
			AllocPage* NewPage = PagePolicy.Allocate();

			// create new blocks from new page
			byte* CurrAddress = NewPage->GetData();		

			H1AllocBlock* NewHead = nullptr;
			H1AllocBlock* BlockTail = (H1AllocBlock*)CurrAddress;

			int32 BlockCount = NewPage->GetSize() / H1AllocBlock::BlockSize;
			for (int32 Index = 0; Index < BlockCount; ++Index)
			{
				H1AllocBlock* NewBlock = (H1AllocBlock*)CurrAddress;
				NewBlock->Layout.Header.Next = NewHead;
				NewHead = NewBlock;

				CurrAddress += H1AllocBlock::BlockSize;
			}

			// link to the head (block) in lock-free
			SGD::Thread::LockFreeStack::Push(FreeBlockHead, NewHead->Layout.Header, BlockTail->Layout.Header);
		}

	protected:
		// free block head
		SGD::Thread::LockFreeStack::H1LfsHead FreeBlockHead;

		// managing the page
		AllocPage* PageHead;

		// alloc page policy type
		AllocPagePolicy PagePolicy;
	};
}
}