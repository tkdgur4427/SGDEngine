#include "H1EnginePrivate.h"
#include "H1WorkerThread.h"

// async logger for thread
#include "H1AsyncLogger.h"

// extern variable initialization
thread_local SGD::Thread::H1WorkerThread_Context* GWorkerThreadContext = nullptr;

using namespace SGD::Thread;

bool H1WorkerUserThread_Imp::Initialize(H1WorkerThread_Context* Context)
{
	return true;
}

uint32 H1WorkerUserThread_Imp::Run(void* Data)
{
	H1WorkerThread_Context* Context = (H1WorkerThread_Context*)Data;
	
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