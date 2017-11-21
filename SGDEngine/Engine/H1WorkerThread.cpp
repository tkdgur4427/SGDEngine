#include "H1EnginePrivate.h"
#include "H1WorkerThread.h"

// extern variable initialization
thread_local SGD::Thread::H1WorkerThread_Context* GWorkerThreadContext = nullptr;

using namespace SGD::Thread;

bool H1WorkerThread_FiberImpl::Initialize(H1WorkerThread_Context* Context)
{
	return true;
}

uint32 H1WorkerThread_FiberImpl::Run(H1WorkerThread_Context* InContext)
{
	H1WorkerThread_Context* Context = InContext;
	
	// initialize worker user thread
	Initialize(Context);

	// marking entrance to memory stack
	SGD::Memory::H1MemMark EntranceMemMark(Context->MemStack);
	
	// running thread
	while (true)
	{
		// marking user thread main loop
		SGD::Memory::H1MemMark MainLoopMemMark(Context->MemStack);
	}
}

// fiber entry point
void StaticFiberEntryPoint(void* InData)
{
	H1FiberThread_Context* FiberContext = (H1FiberThread_Context*)InData;


}

H1FiberThread_Context::H1FiberThread_Context(int32 InStackSize)
	: FiberObject(nullptr)
	, Owner(nullptr)
{
	FiberObject = appCreateFiber(InStackSize, StaticFiberEntryPoint, (byte*)this);
}

H1FiberThread_Context::~H1FiberThread_Context()
{
	appDeleteFiber(FiberObject);
	FiberObject = nullptr;
}