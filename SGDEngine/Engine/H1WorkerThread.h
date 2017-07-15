#pragma once

#include "H1MemStack.h"
#include "H1AsyncLogger.h"

namespace SGD
{
namespace Thread
{
	// forward declaration
	class H1WorkerThread_Context;

	// worker thread logic implementation
	//	- it includes logic for running thread
	//	- it should not have state (any member variables)
	class H1WorkerThread_Impl
	{
	public:		
		virtual uint32 Run(void* Data) = 0;

	protected:
		virtual bool Initialize(H1WorkerThread_Context* Context) = 0;
	};

	// for user thread type (fiber-based worker thread)
	class H1WorkerUserThread_Imp
	{		
	public:
		virtual uint32 Run(void* Data);

	protected:
		virtual bool Initialize(H1WorkerThread_Context* Context);
	};

	// worker thread context definition
	//	- worker thread class only has properties, no logic! 
	class H1WorkerThread_Context
	{
	public:
		// thread handle
		H1ThreadHandleType ThreadHandle;
		// thread id
		H1ThreadIdType ThreadId;

		// extra thread-specific data (like TLS)

		// async logger
		SGD::Log::H1AsyncLogger<char>* Logger;

		// memory stack 
		SGD::Memory::H1MemStack MemStack;

		// allocator (thread-local)

	};	

	// worker thread tuple
	//	- worker thread tuple is not for having member variables, it is only allowed to temporary usage (like scoped life time)
	class H1WorkerThreadTuple
	{
	public:
		H1WorkerThreadTuple()
			: Implementation(nullptr), Context(nullptr)
		{}

		// logic
		H1WorkerThread_Impl* Implementation;
		// state
		H1WorkerThread_Context* Context;
	};

	// worker thread administration
	class H1WorkerThreadAdmin
	{
	public:
		
	};
}
}

// create worker thread context pointer
//	- if GWorkerThreadContext == nullptr, it indicates main thread or rendering thread
extern thread_local SGD::Thread::H1WorkerThread_Context* GWorkerThreadContext;
