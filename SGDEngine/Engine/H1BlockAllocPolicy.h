#pragma once

#include "H1AllocPolicy.h"

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
			struct H1AllocBlockHeader
			{
				H1AllocBlock* Next;	// next block
			};

			enum 
			{
				BlockHeaderSize = SGD::Platform::Util::Align(sizeof(H1AllocBlockHeader), BlockAllocParam::BlockAlignment),
				BlockSize = BlockAllocParam::BlockDataSize + BlockHeaderSize,
			};

			union 
			{
				struct Layout
				{
					byte Data[BlockAllocParam::BlockDataSize];
					H1AllocBlockHeader Header;
				};

				byte Data[BlockSize];
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

		virtual byte* Allocate(uint64 InSize) final
		{
			h1MemCheck(InSize == BlockAllocParam::BlockDataSize, "size should be same as block data size!");

			H1AllocBlock* NewBlock = nullptr;
			H1AllocBlock* NewHead = nullptr;

			// pop the block from the head
			do 
			{
				NewBlock = FreeBlockHead;

				if (NewBlock == nullptr)
				{
					// if the head is nullptr, create new page, but need to repeat from the start so use continue
					CreateNewPage();
					continue;
				}

				NewHead = NewBlock->Header.Next;
			} while (NewBlock != nullptr 
				&& (H1AllocBlock*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&FreeBlockHead, (int64)NewHead, (int64)NewBlock) != NewBlock);

			return NewBlock->Data[0];
		}

		virtual void Deallocate(byte* InPointer) final
		{
			H1AllocBlock* BlockToRemove = (H1AllocBlock*)InPointer;
			H1AllocBlock* CurrHead = nullptr;

			// link removed block to the head
			do 
			{
				CurrHead = FreeBlockHead;
				BlockToRemove->Header.Next = CurrHead;
			} while ((H1AllocBlock*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&FreeBlockHead, (int64)BlockToRemove, (int64)CurrHead) != CurrHead);
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
				NewBlock->Header.Next = NewHead;
				NewHead = NewBlock;

				CurrAddress += H1AllocBlock::BlockSize;
			}

			// link to the head (block) in lock-free
			H1AllocBlock* CurrHead = nullptr;
			do 
			{
				CurrHead = FreeBlockHead;
				BlockTail->Header.Next = CurrHead;
			} while ((H1AllocBlock*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&FreeBlockHead, (int64)NewHead, (int64)CurrHead) != CurrHead);
		}

	protected:
		// free block head
		H1AllocBlock* FreeBlockHead;

		// managing the page
		AllocPage* PageHead;

		// alloc page policy type
		AllocPagePolicy PagePolicy;
	};
}
}