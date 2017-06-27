#pragma once

#include "H1GlobalSingleton.h"
#include "H1Memory.h"
#include "H1RingBuffer.h"
#include "H1PlatformUtil.h"

namespace SGD
{
namespace Log
{
	template <typename CharType>
	struct H1LogNode
	{
		// single linked list node
		H1LogNode* Next;

		// mark node size
		int32 NodeSize;

		// string length
		int32 StrLen;

		// real string data (start pointer)
		CharType Data[0];

		// real string data will be inserted (NodeSize - sizeof(H1LogNode) = StrLen)
	};

	// dedicated log ring buffer
	template <typename CharType>
	struct H1LogRingBuffer
	{
		enum
		{
			NODE_SIZE = sizeof(H1LogNode<CharType>),
		};

		H1LogRingBuffer(byte* BaseAddress, int32 Size)
			, Head(nullptr)
		{	
			// initialize ring buffer layout
			Layout.Initialize(BaseAddress, Size);
		}

		~H1LogRingBuffer()
		{
			
		}

		bool AddLog(const CharType* Message)
		{
			// get the string len
			int32 Len = SGD::Platform::Util::appStrLen(Message);			

			// get node size
			int32 AdditionalSize = Layout.ProduceMaxSize() < NODE_SIZE ? Layout.ProduceMaxSize() : 0;
			int32 NodeSize = NODE_SIZE + AdditionalSize;

			int32 AllocatedSize = NodeSize + Len * sizeof(CharType);

			if (Layout.TotalBufferedSize() < AllocatedSize)
			{
				return false;
			}				

			// allocate to ring buffer
			{
				// first, if there is any additional size to skip, skip it
				if (AdditionalSize)
				{
					Layout.Skip(AdditionalSize);
				}

				// allocate node first (this should be linear memory)
				H1LogNode<CharType>* NewNode = (H1LogNode<CharType>*)Layout.Produce(nullptr, NODE_SIZE);

				// produce string
				int32 MessageSize = Len * sizeof(CharType);
				int32 CurrSize = MessageSize;

				CharType* StartAddress = Layout.Produce(Message, CurrSize);

				// if we need to do more allocation, do one more
				if (CurrSize != MessageSize)
				{
					Layout.Produce((byte*)Message + CurrSize, (MessageSize - CurrSize));
				}

				// update node properties
				NewNode->Data = StartAddress;
				NewNode->NodeSize = NodeSize;
				NewNode->StrLen = Len;
				NewNode->Next = Head;

				// update head pointer
				Head = NewNode;
			}			

			return true;
		}



		// node head
		H1LogNode<CharType>* Head;

		// ring buffer layout
		H1RingBufferLayout Layout;
	};

	/*
		H1AsnycLogger
			- this is logger to leave message asynchronously
			- multi-threaded logging supported
	*/

	class H1AsyncLogger
	{
	public:
	};
}
}