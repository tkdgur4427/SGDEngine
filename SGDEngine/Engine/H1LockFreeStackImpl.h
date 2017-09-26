#pragma once

#include "H1TaggedPointer.h"

namespace SGD
{
namespace Thread
{
namespace LockFreeStack
{
	// preventing naming confusion
	class H1LfsHead : public H1TaggedPointer
	{

	};

	// lock free stack implementation
	void Push(H1LfsHead& Head, H1LfsHead::NodeType* InNode)
	{
		H1LfsHead NewHead;
		H1LfsHead OldHead;
		do
		{
			OldHead = Head;
			InNode->Next = OldHead.GetNode();

			NewHead = OldHead;
			NewHead.SetNode(InNode);
			NewHead.IncrementTag();

		} while ((H1LfsHead)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&Head, (int64)NewHead, (int64)OldHead) != OldHead);
	}

	void Push(H1LfsHead& Head, H1LfsHead::NodeType* InNodeHead, H1LfsHead::NodeType* InNodeTail)
	{
		H1LfsHead NewHead;
		H1LfsHead OldHead;
		do
		{
			OldHead = Head;
			InNodeTail->Next = OldHead.GetNode();

			NewHead = OldHead;
			NewHead.SetNode(InNodeHead);
			NewHead.IncrementTag();

		} while ((H1LfsHead)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&Head, (int64)NewHead, (int64)OldHead) != OldHead);
	}
	}

	H1LfsHead::NodeType* Pop(H1LfsHead& Head)
	{
		H1LfsHead NewHead;
		H1LfsHead OldHead;
		do
		{
			OldHead = Head;
			
			NewHead = OldHead;
			NewHead.SetNode(OldHead.GetNode()->Next);
			NewHead.IncrementTag();

		} while ((H1LfsHead)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&Head, (int64)NewHead, (int64)OldHead) != OldHead);

		return OldHead.GetNode();
	}

	H1LfsHead::NodeType* PopAll(H1LfsHead& Head)
	{
		H1LfsHead NewHead;
		H1LfsHead OldHead;
		do
		{
			OldHead = Head;

			NewHead = OldHead;
			NewHead.SetNode(nullptr);
			NewHead.IncrementTag();

		} while ((H1LfsHead)SGD::Thread::appInterlockedCompareExchange64((volatile int64*)&Head, (int64)NewHead, (int64)OldHead) != OldHead);

		return OldHead.GetNode();
	}
}
}
}