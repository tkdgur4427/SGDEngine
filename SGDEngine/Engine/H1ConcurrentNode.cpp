#include "H1EnginePrivate.h"
#include "H1ConcurrentNode.h"

using namespace SGD::Thread;

// static variable declaration
int32 H1ConcurrentNodeFactory::ConcurrentNodePool::NodeSizes[NodeSizeType::NodeSizeTypeNum] =
{
	1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10,
	1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 19, 1 << 20,
	-1 // NodeSize_Big
};

SGD::Memory::H1BlockAllocatorDefault<H1ConcurrentNodeFactory::ConcurrentNodePage::NodePageSize> H1ConcurrentNodeFactory::ConcurrentNodePage::BlockAllocator;

H1ConcurrentNodeFactory::H1ConcurrentNodeFactory()
{
	for (int32 Index = 0; Index < NodeSizeType::NodeSizeTypeNum; ++Index)
	{
		// set the type num
		NodePools[Index].Type = (NodeSizeType)Index;
	}
}

H1ConcurrentNodeFactory::~H1ConcurrentNodeFactory()
{

}

H1ConcurrentNodeBase* H1ConcurrentNodeFactory::ConcurrentNodePool::AllocatePage()
{
	// create new node page
	ConcurrentNodePage* NewPage = new ConcurrentNodePage(GetNodeSize());

	// link the new page in lock-free
	ConcurrentNodePage* CurrPage = nullptr;

	do 
	{
		CurrPage = PageHead;
		NewPage->NextPage = CurrPage;
	} while ((ConcurrentNodePage*)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&PageHead, (int64)NewPage, (int64)CurrPage) != CurrPage);

	// generate free nodes with new page
	int32 TotalNum = NewPage->GetTotalSize() / NewPage->GetNodeSize();

	int32 Offset = 0;
	byte* StartAddress = NewPage->GetStartAddress();

	H1ConcurrentNodeBase* NewNodeHead = nullptr; // temporary head for newly created for page

	for (int32 Index = 0; Index < TotalNum; ++Index)
	{
		H1ConcurrentNodeBase* CurrNodeBase = (H1ConcurrentNodeBase*)(StartAddress + Offset);

		// set the appropriate data
		CurrNodeBase->TaggedPointer.Pointer = (int64)CurrNodeBase;

		// link the node
		CurrNodeBase->SetNext(NewNodeHead);
		NewNodeHead = CurrNodeBase;

		Offset += NewPage->GetNodeSize();
	}

	return NewNodeHead;
}

H1ConcurrentNodeBase* H1ConcurrentNodeFactory::ConcurrentNodePool::AllocateNodesInternal(int64 Num)
{
	// first get the free head in lock-free stack way
	H1ConcurrentNodeBase* CurrNode = FreeNodeHead;
	for (int64 Index = 0; Index < Num; ++Index)
	{
		// if there is not enough nodes, create new page and attach new free nodes
		if (CurrNode->GetNext() == nullptr)
		{
			H1ConcurrentNodeBase* NewNodes = AllocatePage();
			CurrNode->SetNext(NewNodes);
		}
	}

	// update the links
	FreeHead->FreeNodeHead = CurrNode->GetNext();
	CurrNode->SetNext(nullptr);

	return CurrNode;
}

void H1ConcurrentNodeFactory::ConcurrentNodePool::DeallocateNodesInternal(H1ConcurrentNodeBase* InNode, int64 Num)
{
	// first get the free head in lock-free stack way
	FreeConcurrentNodeWrapper* FreeHead = GetFreeHead();

#if !FINAL_RELEASE && 0
	H1ConcurrentNodeBase* CurrNode = FreeHead->FreeNodeHead;
	for (int64 Index = 0; Index < Num; ++Index)
	{
		CurrNode = CurrNode->GetNext();
	}
	//checkf(CurrNode->GetNext() == nullptr)
#endif
	
	// link the free nodes
	FreeHead->FreeNodeHead->SetNext(InNode);
}