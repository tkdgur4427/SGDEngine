#pragma once

#include "H1MemStack.h"

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
		virtual uint32 Run(H1WorkerThread_Context* InContext) = 0;

	protected:
		virtual bool Initialize(H1WorkerThread_Context* Context) = 0;
	};

	// for user thread type (fiber-based worker thread)
	class H1WorkerThread_FiberImpl : public H1WorkerThread_Impl
	{		
	public:
		virtual uint32 Run(H1WorkerThread_Context* InContext);

	protected:
		virtual bool Initialize(H1WorkerThread_Context* Context);
	};

	// user-thread logic
	class H1FiberThread_ContextImpl
	{
	public:

	};

	// user-thread context
	//	- it captures the running hardware thread context capture (registers, stack, ...)
	class H1FiberThread_Context
	{
	public:

	protected:
		// constructor for fiber-thread context is prohibited to create (only managed by fiber factory)
		H1FiberThread_Context(int32 InStackSize);
		~H1FiberThread_Context();

		// platform-dependent fiber object
		H1FiberHandleType* FiberObject;

		// owner; currently bounded to worker thread instance
		class H1WorkerThread_Context* Owner;
	};

	// worker thread context definition
	//	- worker thread class only has properties, no logic! 
	class H1WorkerThread_Context
	{
	public:

	protected:
		H1WorkerThread_Context();
		~H1WorkerThread_Context();

		// thread handle
		H1ThreadHandleType ThreadHandle;
		// thread id
		H1ThreadIdType ThreadId;

		// extra thread-specific data (like TLS)

		// memory stack 
		SGD::Memory::H1MemStack MemStack;

		// fiber (user-thread context)
		H1FiberThread_Context* FiberContext;
	};	

	// worker thread
	//	- worker thread is not for having member variables, it is only allowed to temporary usage (like scoped life time)
	class H1WorkerThread
	{
	public:
		H1WorkerThread()
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
