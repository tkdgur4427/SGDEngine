#pragma once

#include "H1Memory.h"
#include "H1GlobalSingleton.h"

#include "H1Logger.h"

// block allocator
#include "H1Allocator.h"

namespace SGD
{
namespace Thread
{
	// forward declaration
	class H1ConcurrentNodeFactory;

	/*
		H1ConcurrentNode 
		- memory layout
			- |--H1ConcurrentNode--|--Next(H1ConcurrentNode*)--|--Data--|
	*/
	class H1ConcurrentNodeBase
	{
	public:
		struct H1TaggedPointer
		{
			int64 Tag : 8;		// marker for CAS (ABA problem prevention)
			int64 Pointer : 44; // windows only supports 44 bits addressing anyway
			int64 Unused : 12;
		};

		// the real value structure
		union
		{
			H1TaggedPointer TaggedPointer;
			int64 Data;
		};

		enum
		{
			// node instance size
			NodeInstanceSize = sizeof(H1TaggedPointer),
			// next node size (H1ConcurrentNode*)
			NextNodePointerSize = sizeof(H1ConcurrentNodeBase*),
		};

		// get tag (or mark number)
		int64 GetTag() { return TaggedPointer.Tag; }
		void SetTag(int64 InTag) { TaggedPointer.Tag = InTag; }

		// get next node
		H1ConcurrentNodeBase* GetNext() { return *((H1ConcurrentNodeBase**)((byte*)TaggedPointer.Pointer + NodeInstanceSize)); }

	protected:
		friend class H1ConcurrentNodeFactory;
		H1ConcurrentNodeBase() 
			: Data(0)
		{}

		void SetNext(H1ConcurrentNodeBase* InNextNode) 
		{
			H1ConcurrentNodeBase* NextNode = GetNext();
			SGD::Platform::Util::appMemcpy((byte*)&InNextNode, (byte*)&NextNode, sizeof(void*));
		}
	};

	template <typename DataType>
	class H1ConcurrentNode : public H1ConcurrentNodeBase
	{
	public:		
		// get data methods
		int64 GetDataSize() { return DataSize; }	
		DataType* GetData() { return (DataType*)((byte*)TaggedPointer.Pointer + NextNodePointerSize + NodeInstanceSize); }

	protected:
		enum 
		{
			DataSize = sizeof(DataType),
		};

		// prevent to create concurrent node outside except for 'H1ConcurrentNodeFactory'
		H1ConcurrentNode() 
			: H1ConcurrentNodeBase()
		{}
	};

	class H1ConcurrentNodeFactory
	{
	public:
		static H1ConcurrentNodeFactory* GetSingleton()
		{
			static H1ConcurrentNodeFactory Instance;
			return &Instance;
		}

		template <typename DataType>
		H1ConcurrentNode<DataType>* RequestConcurrentNodes(int64 Num)
		{
			NodeSizeType Type = GetNodeSizeType(sizeof(DataType) + H1ConcurrentNode<DataType>::NextNodePointerSize +
				H1ConcurrentNode<DataType>::NodeInstanceSize);
			
			H1ConcurrentNodeBase* NewNodes = NodePools[Type].AllocateNodes(Num);
			return NewNodes;
		}

		template <typename DataType>
		void ReturnConcurrentNodes(H1ConcurrentNode<DataType>* InNodes, int64 Num)
		{
			NodeSizeType Type = GetNodeSizeType(sizeof(DataType) + H1ConcurrentNode<DataType>::NextNodePointerSize +
				H1ConcurrentNode<DataType>::NodeInstanceSize);

			NodePools[Type].DeallocateNodes(InNodes, Num);
		}

	protected:
		// force to have only one factory instance by GetSingleton
		H1ConcurrentNodeFactory();
		~H1ConcurrentNodeFactory();

		enum NodeSizeType
		{
			// invalid meaning not initialized
			NodeSize_Invalid = -1,

			// byte size for node size type
			NodeSize_16 = 0,
			NodeSize_32,
			NodeSize_64,
			NodeSize_128,
			NodeSize_256,
			NodeSize_512,
			// KB size for node size type
			NodeSize_1KB,
			NodeSize_2KB,
			NodeSize_4KB,

			// currently not supported for this sizes!
			NodeSize_8KB,
			NodeSize_16KB,
			NodeSize_32KB,
			NodeSize_64KB,
			NodeSize_128KB,
			NodeSize_256KB,
			NodeSize_512KB,
			// MB size for node size type
			NodeSize_1MB,
			NodeSize_2MB,
			// indicator for requiring size over 2MB
			NodeSize_Big,
			
			NodeSizeTypeNum,
		};

		// determine node size type based on data size
		NodeSizeType GetNodeSizeType(int64 DataSize)
		{
			int32 CurrBit = 4;
			int64 CurrSize = (int64)1 << CurrBit;

			while (DataSize < CurrSize)
			{
				CurrBit++;
				CurrSize = (int64)1 << CurrBit;
			}

			NodeSizeType Result = (NodeSizeType)(CurrBit - 4);
			h1Check(Result > NodeSizeType::NodeSize_4KB, "currently node size bigger than 4KB is not supported!");

			return Result;
		}

		// concurrent node page wrapping memory block
		//	- as linked-list, it also has NextPage pointer
		struct ConcurrentNodePage
		{
			enum 
			{
				NodePageSize = 4 * 1024, // 4KB page size
			};

			ConcurrentNodePage(int32 InConcurrentNodeSize)
				: Data(nullptr)
				, NextPage(nullptr)
				, ConcurrentNodeSize(InConcurrentNodeSize)
			{
				// allocate page block
				Data = BlockAllocator.Allocate(NodePageSize);
			}

			~ConcurrentNodePage()
			{
				// deallocate memory block
				BlockAllocator.Deallocate(Data);
			}

			byte* GetStartAddress() { return Data; }
			int32 GetTotalSize() { return NodePageSize; }
			int32 GetNodeSize() { return ConcurrentNodeSize; }

			// public variable
			// next page pointer
			ConcurrentNodePage* NextPage;

		protected:
			// concurrent node size
			int32 ConcurrentNodeSize;

			// concurrent node page data pointer
			byte* Data;

		private:
			// block allocator for concurrent node page
			static SGD::Memory::H1BlockAllocatorDefault<NodePageSize> BlockAllocator;
		};

		// concurrent node pool which has type of NodeSizeType
		struct ConcurrentNodePool
		{
			ConcurrentNodePool()
				: Type(NodeSizeType::NodeSize_Invalid)
				, PageHead(nullptr)
				, FreeNodeHead(nullptr)
			{

			}

			int32 GetNodeSize() const { return NodeSizes[Type]; }

			// public methods

			// *Allocate/Deallocate nodes

			// highly recommended to use multiple allocate nodes and deallocate nodes!
			//	- internal implementation is lock-free way (only manipulate using head pointer!)
			H1ConcurrentNodeBase* AllocateNodes(int64 Num) { return AllocateNodesInternal(Num); }
			H1ConcurrentNodeBase* AllocateNode() { return AllocateNodesInternal(1); }

			void DeallocateNodes(H1ConcurrentNodeBase* InNode, int64 Num) { DeallocateNodesInternal(InNode, Num); }
			void DeallocateNode(H1ConcurrentNodeBase* InNode) { DeallocateNodesInternal(InNode, 1); }

			// node size type
			NodeSizeType Type;

			// memory page list
			ConcurrentNodePage* PageHead;

			// static private array for size table for node types
			static int32 NodeSizes[NodeSizeType::NodeSizeTypeNum];

			// free list (lock-free stack way)
			H1ConcurrentNodeBase* FreeNodeHead;

		protected:
			// the methods to use lock-free ways (to reduce thread contention)
			H1ConcurrentNodeBase* AllocateNodesInternal(int64 Num);
			void DeallocateNodesInternal(H1ConcurrentNodeBase* InNode, int64 Num);

		private:
			// watch out, it should not be called other than AllocateNodesInternal!
			// if you really want, please use it carefully (it should be thread-safe and also consider performance-issue)
			H1ConcurrentNodeBase* AllocatePage();
		};

		// node pool type based on node size
		ConcurrentNodePool NodePools[NodeSizeType::NodeSizeTypeNum];
	};
}
}