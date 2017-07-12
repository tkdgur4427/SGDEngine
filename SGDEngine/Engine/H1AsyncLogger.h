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

		// tracking skip size (to restore original state)
		int32 SkipSize;

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
			: Head(nullptr)
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
				// not enough memory return false
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
				NewNode->SkipSize = AdditionalSize;
				NewNode->StrLen = Len;
				NewNode->Next = Head;

				// update head pointer
				Head = NewNode;
			}			

			return true;
		}

		bool RemoveLog(CharType* Message, int32 MessageSize)
		{
			// first get the head
			H1LogNode<CharType>* RemoveNode = Head;

			if (Message == nullptr || MessageSize < RemoveNode->StrLen)
			{
				// not enough message is allocated, so failed to retrieve message
				return false;
			}
			
			// copy the message
			SGD::Platform::Util::appMemcpy(Message, RemoveNode->Data, RemoveNode->StrLen);

			// consume the ring buffer and update the head node ptr
			int32 CurrConsumedSize = RemoveNode->StrLen;
			Layout.Consume(nullptr, CurrConsumedSize);

			// if the consuming is clamped, one more consume
			if (CurrConsumedSize != RemoveNode->StrLen)
			{
				Layout.Consume(nullptr, RemoveNode->StrLen - CurrConsumedSize);
			}

			// consume node itself (node is guaranteed linear)
			Layout.Consume(nullptr, RemoveNode->NodeSize);

			// consume skip size, if there is
			if (RemoveNode->SkipSize > 0)
			{
				Layout.Consume(nullptr, SkipSize);
			}

			return true;
		}

		// node head
		H1LogNode<CharType>* Head;

		// ring buffer layout
		SGD::Util::H1RingBufferLayout Layout;
	};

	/*
		H1AsnycLogger
			- this is logger to leave message asynchronously
			- multi-threaded logging supported

			- adapt the design for 'observer pattern' (subscriber - publisher?)
	*/
	
	// forward declaration
	class H1AsyncLoggerAdmin;

	struct H1AsyncLoggerNode
	{
		// async logger log character type
		enum AsyncLoggerType
		{
			CHAR, // char
			// ...
		};

		H1AsyncLoggerNode()
			: Next(nullptr)
		{}

		// binded thread id
		H1ThreadIdType ThreadId;
		// async logger type depending on character type
		AsyncLoggerType Type;
		// next ptr
		H1AsyncLoggerNode* Next;
	};

	// subscribers for async logger
	template <class CharType>
	class H1AsyncLogger : public H1AsyncLoggerNode
	{
	public:
		enum 
		{
			RingBufferSize = 512 * 1024, // 512 KB
		};

		H1AsyncLogger(byte* BaseAddress, int32 Size)
			: RingBuffer(BaseAddress, Size)
		{
			// setting async logger type to convert correctly
			Type = GetAsyncLoggerType();
		}

		AsyncLoggerType GetAsyncLoggerType()
		{
			AsyncLoggerType Result;

			if (sizeof(CharType) == sizeof(char))
			{
				Result = AsyncLoggerType::CHAR;
			}

			return Result;
		}

		// data (ring buffer format)
		H1LogRingBuffer<CharType> RingBuffer;
	};

	template <typename CharType>
	class H1AsyncLoggerRegisterator
	{
	public:
		static bool Register();
		static bool Unregister();
	};

	// admin to manage multiple async logger and dump logs
	class H1AsyncLoggerAdmin
	{
	public:
		// template friend class declaration
		template <typename CharType>
		friend class H1AsyncLoggerRegisterator;

		H1AsyncLoggerAdmin()
			: Head(nullptr)
		{}

	protected:
		// for preventing duplication checking of async logger for same thread id
		bool IsExistAsyncLoggerByTheadId(H1ThreadIdType ThreadId)
		{
			// whether current thread already binded to async logger
			bool Result = false;

			while (Head != nullptr)
			{
				if (Head->ThreadId == ThreadId)
				{
					Result = true;
					break;
				}

				Head = Head->Next;
			}

			return Result;
		}

		// async logger node linked list
		H1AsyncLoggerNode* Head;
	};

	template <typename CharType>
	bool H1AsyncLoggerRegisterator<CharType>::Register()
	{
		H1WorkerThread_Context* CurrThreadContext = nullptr;

		// getting current thread context
		if (GWorkerThreadContext == nullptr)
		{
			CurrThreadContext = GMainThreadContext;
		}
		else
		{
			CurrThreadContext = GWorkerThreadContext;
		}

		// check whether the async logger already exists
		bool bIsExist = H1GlobalSingleton::AsyncLoggerAdmin()->IsExistAsyncLoggerByTheadId(CurrThreadContext->ThreadId);
		if (bIsExist == true)
		{
			return false;
		}

		// first allocating ring buffer
		int32 Size = H1AsyncLogger<CharType>::RingBufferSize;
		byte* BaseAddress = GWorkerThreadContext->MemStack.Push(Size);

		// next allocating S1AsnycLogger itself to the memory stack
		H1AsyncLogger<CharType>* NewLogger = (H1AsyncLogger*)GWorkerThreadContext->MemStack.Push(sizeof(H1AsyncLogger));
		*NewLogger = H1AsyncLogger<CharType>(BaseAddress, Size);

		// setting new logger properties
		NewLogger->ThreadId = CurrThreadContext->ThreadId;

		// link to the admin
		NewLogger->Next = H1GlobalSingleton::AsyncLoggerAdmin()->Head;
		H1GlobalSingleton::AsyncLoggerAdmin()->Head = NewLogger;

		return true;
	}

	template <typename CharType>
	bool H1AsyncLoggerRegisterator<CharType>::Unregister()
	{
		H1WorkerThread_Context* CurrThreadContext = nullptr;

		// getting current thread context
		if (GWorkerThreadContext == nullptr)
		{
			CurrThreadContext = GMainThreadContext;
		}
		else
		{
			CurrThreadContext = GWorkerThreadContext;
		}

		// check whether the async logger exists
		bool bIsExist = H1GlobalSingleton::AsyncLoggerAdmin()->IsExistAsyncLoggerByTheadId(CurrThreadContext->ThreadId);
		if (bIsExist == false)
		{
			return false;
		}



		return true;
	}
}
}