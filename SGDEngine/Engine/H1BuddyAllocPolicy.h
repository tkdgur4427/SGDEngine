#pragma once

#include "H1PlatformThread.h"
#include "H1Logger.h"

namespace SGD
{
namespace Memory
{
	template <uint64 Size, uint64 LeafSize, uint64 Alignment = 16>
	class H1BuddyAllocParams
	{
	public:
		enum
		{
			// total size of the buffer for buddy allocator
			TotalSize = SGD::Platform::Util::PowerOfTwo(Size),
			// leaf node size of block (it should be aligned as well as power of two)
			LeafBlockSize = SGD::Platform::Util::PowerOfTwo(SGD::PlatformLeafSize::Util::Align(LeafSize, Alignment)),
		};

		H1BuddyAllocParams()
		{
#if !FINAL_RELEASE
			ThreadId = SGD::Thread::appGetCurrentThreadId();
#endif
		}

	protected:
#if !FINAL_RELEASE
		bool IsRunInSameThead()
		{
			return (ThreadId == SGD::Thread::appGetCurrentThreadId());
		}

		// buddy allocator didn't support multi-threaded (lock-free) support
		// recommended only for thread-local way!
		uint64 ThreadId;
#endif
	};

	// for buddy alloc policy to fast look-up table to reduce the computation time complexity
	template <uint64 MaxSize, uint64 MinSize, uint64 MaxLevel>
	class H1BuddyAllocLookupTables
	{
	public:
		H1BuddyAllocLookupTables()
		{
			// compute tables
			ComputeLevelIndexToBlockIndexTable();
		}

		uint64 GetLevelIndexToBlockIndex(uint64 InLevelIndex)
		{
			return LevelIndexToBlockIndex[InLevelIndex];
		}

		uint64 GetLevelIndexToBlockSize(uint64 InLevelIndex)
		{
			return LevelIndexToBlockSize[InLevelIndex];
		}

		// note that even though it is binary search, if the level num is big, it also inefficient
		// (but we can't make the lookup table for block which is too wasteful in memory space)
		uint64 GetBlockIndexToLevelIndex(uint64 InBlockIndex)
		{
			for (uint64 LevelIndex = 0; LevelIndex < LevelNum - 1; ++LevelIndex)
			{
				if (LevelIndexToBlockIndex[LevelIndex + 1] > InBlockIndex)
				{
					return LevelIndex;
				}
			}

			// it reach to the end of level
			return LevelNum - 1;
		}

		uint64 GetBlockSizeToLevelIndex(uint64 InBlockSize)
		{
			for (uint64 LevelIndex = 0; LevelIndex < LevelNum - 1; ++LevelIndex)
			{
				if (LevelIndexToBlockSize[LevelIndex + 1] > InBlockSize)
				{
					return LevelIndex;
				}
			}

			// it reach to the end of level
			return LevelNum - 1;
		}

	protected:
		void ComputeLevelIndexToBlockIndexTable()
		{
			// first calculate the level num
			LevelNum = 0;
			uint64 CurrBlocks = MaxSize / MinSize;

			while (CurrBlocks > 0)
			{
				CurrBlocks >>= 1;
				LevelNum++;
			}

			h1Check(LevelNum < MaxLevel, "it reaches maximum level, please check");

			// with the calculated level num, fill out the table
			uint64 BlockIndex = 0;
			uint64 CurrSize = MaxSize;

			for (int64 LevelIndex = 0; LevelIndex < LevelNum; ++LevelIndex)
			{
				LevelIndexToBlockIndex[LevelIndex] = BlockIndex;
				LevelIndexToBlockSize[LevelIndex] = CurrSize;
				BlockIndex += (1 << LevelIndex);
				CurrSize >>= 1;
			}
		}

		int64 LevelIndexToBlockIndex[MaxLevel];
		int64 LevelIndexToBlockSize[MaxLevel];

		// the calculated total level count
		int64 LevelNum;
	};

	template <class BuddyAllocParams, class AllocPagePolicy = H1DefaultAllocOneLargePagePolicy>
	class H1BuddyAllocPolicy : public BuddyAllocParams, public H1AllocPolicy<AllocPagePolicy>
	{
	public:
		H1BuddyAllocPolicy()
			: PagePolicy(BuddyAllocParams::TotalSize)
		{
			// allocate one large memory page
			PagePolicy.Allocate();

			// initialize
			Initialize();
		}

		virtual ~H1BuddyAllocPolicy()
		{
			// no need to deallocate one large page (unique_ptr)
		}

		void* Allocate(uint64 InSize)
		{
			return nullptr;
		}

		void Deallocate(void* InPointer)
		{
			
		}

	protected:
		// one large page allocation policy
		AllocPagePolicy PagePolicy;

		// for each buddy block, it has its own state 
		enum EBlockState
		{
			BlockState_Divided		= 1 << 2,	// the block is divided which means one of sub-block is already allocated
			BlockState_Allocated	= 1 << 1,	// the block is allocated
			BlockState_Freed		= 0,		// the block is free now
		};

		class H1BuddyChunk
		{
		public:
			H1BuddyChunk()
				: Data(0xFFFFFFFFFFFFFFFF)
			{}

			enum
			{
				BlockNum = 8,	// total 8 element exists in one buddy block
			};

			union
			{
				// it reflects enumeration EBlockState
				byte BlockStates[BlockNum];
				// layout as 'uint64' which is the main reason that each chunk has 8 blocks
				uint64 Data;
			};

			inline bool IsAllAllocated() { return (Data == 0x0101010101010101); }
			inline bool IsAllNotSet() { return (Data == 0xFFFFFFFFFFFFFFFF) }
		};

		enum
		{
			// local enumeration for this buddy alloc class
			LevelNum = CalculateLevelNum(),
			BlockNum = CalculateBlockNum(LevelNum),
			ChunkNum = (BlockNum + (H1BuddyChunk::BlockNum - 1)) / H1BuddyChunk::BlockNum;
		};

		uint64 GetBlockIndexFromLevelIndex(uint64 InLevelIndex)
		{
			return LookupTable.GetBlockIndexToLevelIndex(InLevelIndex);
		}

		uint64 GetLevelIndexFromBlockIndex(uint64 InBlockIndex)
		{
			return LookupTable.GetLevelIndexToBlockIndex(InBlockIndex);
		}

		uint64 GetBuddyBlockSize(uint64 InSize)
		{
			return SGD::Platform::Util::PowerOfTwo(InSize);
		}

		uint64 GetLevelIndexFromSize(uint64 InSize)
		{
			h1Check(InSize == SGD::Platform::Util::PowerOfTwo(InSize), "the size should be power of two!");s
			return LookupTable.GetLevelIndexToBlockSize(InSize);
		}

		bool DivideBuddyBlocks(uint64 InLevelIndex, uint64 InBlockIndex)
		{
			// first find the free block on higher level
			uint64 EndLevelIndex = InLevelIndex;

			uint64 CurrLevelIndex = InLevelIndex;
			uint64 StartBlockIndexForCurrLevel = GetBlockIndexFromLevelIndex(StartLevelIndex);
			uint64 OffsetBlockIndexForCurrLevel = InBlockIndex - StartBlockIndexForCurrLevel;

			uint64 StartLevelIndex = 0;
			while (OffsetBlockIndexForCurrLevel > 0)
			{
				StartBlockIndexForCurrLevel = GetBlockIndexFromLevelIndex(CurrLevelIndex);

				uint64 BlockIndex = StartBlockIndexForCurrLevel + OffsetBlockIndexForCurrLevel;
				uint64 ChunkIndex = BlockIndex / H1BuddyChunk::BlockNum;
				uint64 SubIndex = BlockIndex % H1BuddyChunk::BlockNum;

				if (BuddyChunks[ChunkIndex].BlockStates[SubIndex] == EBlockState::BlockState_Freed)
				{
					// when it founds the free block, set the start level and index and out of the loop
					CurrLevelIndex++;
					OffsetBlockIndexForCurrLevel <<= 1;

					StartLevelIndex = CurrLevelIndex;

					// set the block state as divided
					BuddyChunks[ChunkIndex].BlockStates[SubIndex] = EBlockState::BlockState_Divided;

					break;
				}

				// update the level index and offset block index
				CurrLevelIndex--;
				OffsetBlockIndexForCurrLevel >>= 1;
			}

			if (StartLevelIndex == 0)
			{
				// failed to find the proper buddy block which means failed to allocate
				return false;
			}

			// second start to divide the block
			uint64 CurrBlockIndexOffset = OffsetBlockIndexForCurrLevel;

			for (uint64 LevelIndex = StartLevelIndex; LevelIndex < EndLevelIndex; ++LevelIndex)
			{
				// change the state of current level
				uint64 StartBlockIndex = GetBlockIndexFromLevelIndex(LevelIndex);
				uint64 CurrBlockIndex = StartBlockIndex + CurrBlockIndexOffset;

				uint64 ChunkIndex = CurrBlockIndex / 8;
				uint64 SubIndex = CurrBlockIndex % 8;

				// set the block state as divided
				h1Check(BuddyChunks[ChunkIndex].BlockStates[SubIndex] == EBlockState::BlockState_Freed, "the block state before dividing, it should be free state!");
				BuddyChunks[ChunkIndex].BlockStates[SubIndex] = EBlockState::BlockState_Divided;
				BuddyChunks[ChunkIndex].BlockStates[SubIndex + 1] = EBlockState::BlockState_Freed;

				// update block index offset
				CurrBlockIndexOffset <<= 1;
			}

			// when it reaches to the end of level, handle it separately
			h1Check(CurrBlockIndexOffset == OffsetBlockIndexForCurrLevel, "it should be same!");

			// change the state of current level
			uint64 StartBlockIndex = GetBlockIndexFromLevelIndex(EndLevelIndex);
			uint64 CurrBlockIndex = StartBlockIndex + CurrBlockIndexOffset;
			h1Checkf(CurrBlockIndex == InBlockIndex);

			uint64 ChunkIndex = CurrBlockIndex / 8;
			uint64 SubIndex = CurrBlockIndex % 8;

			// set the block state as divided
			h1Check(BuddyChunks[ChunkIndex].BlockStates[SubIndex] == EBlockState::BlockState_Divided, "at the end of level, it should be divided");
			BuddyChunks[ChunkIndex].BlockStates[SubIndex] = EBlockState::BlockState_Allocated;
			BuddyChunks[ChunkIndex].BlockStates[SubIndex + 1] = EBlockState::BlockState_Freed;

			return true;
		}

		uint64 AllocateBuddyBlock(uint64 InSize)
		{
			// first calculate buddy block size
			uint64 BuddyBlockSize = GetBuddyBlockSize(InSize);

			// get the level index
			uint64 LevelIndex = GetLevelIndexFromSize(BuddyBlockSize);

			// get the block base block index
			uint64 StartBlockIndex = GetBlockIndexFromLevelIndex(LevelIndex);
			uint64 EndBlockIndex = GetBlockIndexFromLevelIndex(LevelIndex + 1);

			for (uint64 BlockIndex = StartBlockIndex; BlockIndex < EndBlockIndex; ++BlockIndex)
			{
				uint64 ChunkIndex = BlockIndex / H1BuddyChunk::BlockNum;
				uint64 SubIndex = BlockIndex % H1BuddyChunk::BlockNum;

				// if current buddy block is all allocated, skip the chunk (blocks)
				if (BuddyChunks[ChunkIndex].IsAllAllocated())
				{
					BlockIndex += (H1BuddyChunk::BlockNum - SubIndex);
					continue;
				}

				// if the block is allocated skip it
				if (BuddyChunks[ChunkIndex].BlockStates[SubIndex] == EBlockState::BlockState_Allocated)
				{
					continue;
				}

				// if block is free, give the current block index;
				if (BuddyChunks[ChunkIndex].BlockStates[SubIndex] == EBlockState::BlockState_Freed)
				{
					h1Check(SubIndex % 2 == 1, "in buddy allocator, block_freed flag should be in odd index");

					// change the block state and return the block index
					BuddyChunks[ChunkIndex].BlockStates[SubIndex] = EBlockState::BlockState_Allocated;
					return BlockIndex;
				}

				else
				{
					h1Check(SubIndex % 2 == 0, "in buddy allocator, 0xFF (not set flag) should be in even index");
					h1Check(BuddyChunks[ChunkIndex].BlockStates[SubIndex] == 0xFF, "its block state should be not set!");

					// divide the bock
					DivideBuddyBlocks(LevelIndex, BlockIndex);
					return BlockIndex;
				}
			}

			// not available buddy block exists! ; failed to allocate the block
			return -1;
		}

		void MergeBuddyBlocks()
		{

		}

		void DeallocateBuddyBlock(uint64 InOffset)
		{

		}

		// array of buddy chunks
		H1BuddyChunk BuddyChunks[ChunkNum];

		// look up table
		H1BuddyAllocLookupTables<TotalSize, LeafBlockSize, 128> LookupTable;

	private:
		void Initialize()
		{
			// set the first buddy block (level0) as free
			BuddyChunks[0].BlockStates[0] = EBlockState::BlockState_Freed;
		}

		// const expression methods

		// calculate buddy chunk count in compile-time
		constexpr uint64 CalculateLevelNum()
		{
			uint64 CurrBlockNum = TotalSize / LeafBlockSize;
			uint64 Result = 0;

			while (CurrBlockNum > 0)
			{
				Result++;
				CurrBlockNum <<= 1;
			}

			return Result;
		}

		// calculate block number based on level num
		constexpr uint64 CalculateBlockNum(uint64 InLevelNum)
		{
			uint64 Result = 0;
			for (uint64 LevelIndex = 0; LevelIndex < InLevelNum; ++LevelIndex)
			{
				Result += (1 << LevelIndex);
			}
			return Result;
		}
	};
}
}
