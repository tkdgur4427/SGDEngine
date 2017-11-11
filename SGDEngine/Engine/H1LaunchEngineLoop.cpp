#include "H1EnginePrivate.h"
#include "H1LaunchEngineLoop.h"

#include "H1WorkerThread.h"

// declaring main thread context
SGD::Thread::H1WorkerThread_Context GMainThreadContext;

void Init()
{
	// initialize main thread context
	GMainThreadContext.ThreadId = SGD::Thread::appGetCurrentThreadId();
	GMainThreadContext.ThreadHandle = nullptr; // indicating main thread

	// setting GWorkerThreadContext as main thread
	GWorkerThreadContext = &GMainThreadContext;
}

void Run()
{
	// scope marking memory stack in GMainThreadContext
	SGD::Memory::H1MemMark MainThreadRunMark(GMainThreadContext.MemStack);

	struct Temp
	{
		Temp() : a(0), b(0), c(0), d(0) {}
		int32 a, b, c, d;
	};

	Temp* pTemp = new (GWorkerThreadContext->MemStack) Temp;
	pTemp->a = 100;
	pTemp->b = 10;

	Temp* aTemps = new (GWorkerThreadContext->MemStack) Temp[10];
	aTemps[5].a = 100;
}

void Destroy()
{

}