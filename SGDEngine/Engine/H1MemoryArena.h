#pragma once
namespace SGD
{
namespace Memory
{
	/*
		Memory Block
			- H1MemoryBlock is the external interface as output of memory allocation from Memory Arena
	*/
	class H1MemoryBlock
	{
	public:
		byte* BaseAddress;

	protected:
		// page id to lookup the memory page in MemoryArena
		int32 PageTagId;
		// block offset to lookup the block
		int32 Offset;
	};


	// if it requires multiple contiguous memory block
	class H1MemoryBlockRange
	{
	public:
		byte* BaseAddress;

	protected:
		int32 PageTagId;
		// block offset and count (contiguous block count)
		byte Offset;
		byte Count;
	};

	// if it requires more than 126 MB memory size, externally allocate this large block
	//	- it is not called that frequently!
	class H1MemoryLargeBlock
	{
	public:
	};

	/*
		Memory Arena
			- we can think of Memory Arena as the starting point (head quarter) for memory allocation
	*/
	class H1MemoryArena
	{
	public:
		H1MemoryArena() {}
		~H1MemoryArena() {}

	protected:
		enum { 
			MEMORY_BLOCK_SIZE = 2 * 1024 * 1024, // memory block size is 2 MB
		};

		// the memory header that includes all information for memory allocation
		struct MemoryHeader
		{
			// memory header should be less than 32KB (total size)
			byte Offset; // memory block offset
		};

		// the wrapper of actual memory data
		struct MemoryBlock
		{
			// actual data sized by MEMORY_BLOCK_SIZE
			byte Data[MEMORY_BLOCK_SIZE];
		};

		/*
			Memory Page
				- memory page contains memory headers and memory blocks
				- this memory page is controlled by memory arena
				- you can think of this memory page as virtual memory page in OS
		*/
		struct MemoryPage
		{
			MemoryPage() {}
			~MemoryPage() {}

			enum
			{
				// we have 63 memory block counts for actual usage; memory page provide us total 126MB to use
				MEMORY_BLOCK_COUNT = 63,
				// last memory block is sued for headers and additional properties, so the block make it empty
				ALLOC_BIT_MASK_FULL = (uint64)(0xFFFFFFFFFFFFFFFF >> 1),
			};
			
			// tagger for memory page 
			//	- has additional information for this memory page
			//	- we can use this tag for indicator to distinguish the usages
			//	- tag is contained as unique id
			//		- like Default + N ( N < Default2 for example)
			enum Tag
			{
				Default = 0,
				// Default2 = 1 << 2, ... etc
			};

			// memory page layout to align memory blocks below union structure
			struct MemoryPageLayout
			{
				MemoryBlock		MemoryBlocks[MEMORY_BLOCK_COUNT];

				// below section should be less than 2MB (MEMORY_BLOCK_SIZE)
				MemoryHeader	Headers[MEMORY_BLOCK_COUNT];	// memory headers
				// properties
				// 1. alloc bit mask
				uint64			AllocBitMask;
				// 2. singly linked list (tracking next page and next free page)
				eastl::unique_ptr<MemoryPage> NextPage;
				MemoryPage*	NextFreePage;
				// 3. unique id
				//	- memory page cannot over the range of uint32 (it will over TB...)
				uint32			TagId;
			};

			union
			{
				// memory page layout
				MemoryPageLayout Layout;

				// memory blocks
				//	- last memory block is used for the container for memory headers
				MemoryBlock MemoryBlocks[MEMORY_BLOCK_COUNT + 1];
			};

			// methods
			bool IsFull() const { return Layout.AllocBitMask != ALLOC_BIT_MASK_FULL; }
			MemoryPage* GetNextPage() { return Layout.NextPage.get(); }
			MemoryPage* GetNextFreePage() { return Layout.NextFreePage; }

			// allocate/deallocate (for internal methods for MemoryPage)

			// explicitly define input and output structure to allocate/deallocate memory for separating dependencies between MemoryArena class
			// 1. allocate params
			struct AllocInput
			{
				// requested memory block count
				int32 BlockCount; 
			};

			struct AllocOutput
			{				
				// memory page id (tag id)
				uint32 TagId;
				// memory block offset and count
				int32 Offset;
				int32 Count;
			};

			// 2. deallocate params
			struct DeallocInput
			{
				// memory block offset and count
				int32 Offset;
				int32 Count;
			};

			AllocOutput Allocate(const AllocInput& Params);
			void Deallocate(const DeallocInput& Params);

		protected:
			// internal helper methods

			// get available memory block index
			int32 GetAvailableBlockIndex(int32 InBlockCount = 1);
			// marking alloc bits with Value
			void MarkAllocBits(bool InValue, int32 InOffset, int32 InCount = 1);
			// validation checking
			void ValidateAllocBits(bool InValue, int32 InOffset, int32 InCount = 1);

		};

		// memory pages
		eastl::unique_ptr<MemoryPage> PageHead;
		MemoryPage*	FreePageHead;
	};
}
}