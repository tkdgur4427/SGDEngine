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

	// register thread-local logger


	// setting GWorkerThreadContext as main thread
	GWorkerThreadContext = &GMainThreadContext;
}

void Run()
{
	// scope marking memory stack in GMainThreadContext
	SGD::Memory::H1MemMark MainThreadRunMark(GMainThreadContext.MemStack);


}

void Destroy()
{

}