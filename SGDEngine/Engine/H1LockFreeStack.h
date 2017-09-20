#pragma once

#include "H1ConcurrentNode.h"

namespace SGD
{
namespace Thread
{
	template <typename DataType>
	class H1LockFreeStack
	{
	public:
		H1LockFreeStack()
			: EmptyHead(ConcurrentNodeWrapper::Empty)
			, NormalHead(ConcurrentNodeWrapper::Normal)
			, EmptyFreeNodes(ConcurrentNodeWrapper::Empty)
			, NormalFreeNodes(ConcurrentNodeWrapper::Normal)
			, Head(&EmptyHead)
			, FreeNodes(&EmptyFreeNodes)
			, CurrRequestNum(InitialRequestNodeNum)
		{
			Initialize();
		}

		~H1LockFreeStack()
		{
			Destroy();
		}

		void Push(DataType& InData)
		{
			ConcurrentNodeWrapper* TempFreeNodes = GetFreeNodes();
			EnsureFreeNodeSize(TempFreeNodes);
			
			// create new node
			H1ConcurrentNode<DataType>* NewNode = TempFreeNodes->Head;
			TempFreeNodes->Head = NewNode->GetNext();

			ReturnFreeNodes();

			// unlink new node from head
			NewNode->SetNext(nullptr);
			
			// set new data
			*(NewNode->GetData()) = InData;

			// increase the tag number by 1 (ABA problem)
			NewNode->GetTag()++;

			ConcurrentNodeWrapper* TempHead = GetHead();

			// now start to push the element with the new node
			NewNode->SetNext(TempHead->Head);
			TempHead->Head = NewNode;

			ReturnHead();
		}

		DataType Pop()
		{
			ConcurrentNodeWrapper* TempHead = GetHead();

			// get the data to return
			DataType OutData = *(TempHead->Head->GetData());
			
			// unlink the head node
			H1ConcurrentNode<DataType>* RemovedNode = TempHead->Head;
			TempHead->Head = RemovedNode->GetNext();
			RemovedNode->SetNext(nullptr);

			// reset the data value
			//*(RemovedNode->GetData()) = DataType();

			ConcurrentNodeWrapper* TempFreeNodes = GetFreeNodes();

			// link removed node to free nodes
			RemovedNode->SetNext(TempFreeNodes->Head);
			TempFreeNodes->Head = RemovedNode;

			return OutData;
		}

		void PopAll()
		{
			ConcurrentNodeWrapper* TempHead = GetHead();

			// get the all nodes
			H1ConcurrentNode<DataType>* RemovedNodes = TempHead->Head;
			TempHead->Head = nullptr;

			H1ConcurrentNode<DataType>* LastNodeForRemovedNodes = RemovedNodes;
			while (LastNodeForRemovedNodes->GetNext() != nullptr)
			{
				//*(LastNodeForRemovedNodes->GetData()) = DataType();
				LastNodeForRemovedNodes = LastNodeForRemovedNodes->GetNext();
			}

			ConcurrentNodeWrapper* TempFreeNodes = GetFreeNodes();

			// link all removed nodes to the free nodes
			LastNodeForRemovedNodes->SetNext(TempFreeNodes->Head);
			TempFreeNodes->Head = RemovedNodes;
		}

		bool IsEmpty()
		{
			ConcurrentNodeWrapper* TempHead = GetHead();
			return TempHead->Head == nullptr;
		}

	protected:
		void Initialize()
		{

		}

		void Destroy()
		{
			PopAll();

			int64 Count = 0;
			H1ConcurrentNode<DataType>* CurrNode = NormalFreeNodes->Head;
			while (CurrNode->GetNext() != nullptr)
			{
				// reset the tag
				CurrNode->GetTag() = 0;

				Count++;
			}

			// returning all nodes requested from node factory
			H1ConcurrentNodeFactory::GetSingleton->ReturnConcurrentNodes(CurrNode, Count);
		}

		ConcurrentNodeWrapper* GetHead()
		{
			ConcurrentNodeWrapper* Result = nullptr;
			while (true)
			{
				Result = (ConcurrentNodeWrapper*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Head), (int64)(&EmptyHead), (int64)(&NormalHead));
				if (Result == &NormalHead)
				{
					break;
				}
			}

			return Result;
		}

		void ReturnHead()
		{
			ConcurrentNodeWrapper* Result = (ConcurrentNodeWrapper*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(Head), (int64)(&NormalHead), (int64)(&EmptyHead));			
		}

		ConcurrentNodeWrapper* GetFreeNodes()
		{
			ConcurrentNodeWrapper* Result = nullptr;
			while (true)
			{
				Result = (ConcurrentNodeWrapper*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(FreeNodes), (int64)(&EmptyFreeNodes), (int64)(&NormalFreeNodes));
				if (Result == &NormalFreeNodes)
				{
					break;
				}
			}

			return Result;
		}

		void ReturnFreeNodes()
		{
			ConcurrentNodeWrapper* Result = (ConcurrentNodeWrapper*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&(FreeNodes), (int64)(&NormalFreeNodes), (int64)(&EmptyFreeNodes));
		}

		struct ConcurrentNodeWrapper
		{
			enum WrapperType
			{
				Normal,
				Empty,
			};

			ConcurrentNodeWrapper(WrapperType InType)
				: Head(nullptr)
				, Type(InType)
			{}

			WrapperType Type;
			H1ConcurrentNode<DataType>* Head;
		};

		// the real head for concurrent stack
		ConcurrentNodeWrapper* Head;
		ConcurrentNodeWrapper NormalHead;
		ConcurrentNodeWrapper EmptyHead;

		// free nodes
		H1ConcurrentNode<DataType>* FreeNodes;
		ConcurrentNodeWrapper NormalFreeNodes;
		ConcurrentNodeWrapper EmptyFreeNodes;

		enum 
		{
			InitialRequestNodeNum = 4,	// initial request node number is 4
		};

		// increment size to request free nodes
		int64 CurrRequestNum;

	private:
		void EnsureFreeNodeSize(ConcurrentNodeWrapper* InFreeNodes)
		{
			if (InFreeNodes->Head == nullptr)
			{
				// request number of concurrent nodes
				H1ConcurrentNode<DataType>* NewNodes = H1ConcurrentNodeFactory::GetSingleton()->RequestConcurrentNodes<DataType>(CurrRequestNum);

				// link the new nodes to the head
				InFreeNodes->Head = NewNodes;

				// increase the request number (by x2)
				CurrRequestNum <<= 1;
			}
		}
	};
}
}