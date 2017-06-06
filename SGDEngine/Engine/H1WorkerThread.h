#pragma once

namespace SGD
{
namespace Thread
{
	// worker thread logic implementation
	//	- it includes logic for running thread
	//	- it should not have state (any member variables)
	class H1WorkerThread_Impl
	{
	public:
		virtual uint32 Run(void* Data) = 0;
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
	};

	// worker thread tuple
	//	- worker thread tuple is not for having member variables, it is only allowed to temporary usage (like scoped life time)
	class H1WorkerThreadTuple
	{
	public:
		H1WorkerThreadTuple()
			: Implementation(nullptr), Context(nullptr)
		{}

		H1WorkerThread_Impl* Implementation;
		H1WorkerThread_Context* Context;
	};

	// worker thread administration
	class H1WorkerThreadAdmin
	{
	public:

	};
}
}
