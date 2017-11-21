#pragma once

// thread-related types
typedef void* H1ThreadHandleType;
typedef uint32 H1ThreadIdType;

// fiber-related types
typedef void* H1FiberHandleType;

// windows platform (PC)
#if SGD_WINDOWS_PLATFORM
typedef DWORD (WINAPI *H1ThreadEntryPoint)(LPVOID lpThreadParameter);
typedef void (CALLBACK *H1FiberEntryPoint)(LPVOID lpFiberparameter);
#else
// unhandled!
#endif

namespace SGD
{
namespace Thread
{
	// utility functions (platform-dependent) for thread 

	// thread creation
	struct CreateThreadOutput
	{
		H1ThreadHandleType ThreadHandle;
		H1ThreadIdType ThreadId;		
	};

	struct CreateThreadInput
	{
		uint32 StackSize;
	};

	bool appCreateThread(CreateThreadOutput& Output, const CreateThreadInput& Input, H1ThreadEntryPoint ThreadEntryPoint, bool bResume = true);

	// thread destruction
	bool appDestroyThread(H1ThreadHandleType ThreadHandle);

	// join thread
	struct JointThreadInput
	{
		int32 NumThreads;
		H1ThreadHandleType* ThreadArray;
	};

	void appJoinThreads(const JointThreadInput& Input);

	// get current thread id
	H1ThreadIdType appGetCurrentThreadId();

	// set thread affinity
	void appSetThreadAffinity(H1ThreadHandleType ThreadHandle, uint32 CPUCoreId);

	// sleep
	void appSleep(int32 MilliSeconds = 0);

	// interlocked methods
	int32 appInterlockedCompareExchange32(volatile int32* Dest, int32 Exchange, int32 Comperand);
	int64 appInterlockedCompareExchange64(volatile int64* Dest, int64 Exchange, int64 Comperand);

	// fiber methods
	H1FiberHandleType* appCreateFiber(int32 InStackSize, H1FiberEntryPoint InFiberEntryPoint, byte* InData);
	void appDeleteFiber(H1FiberHandleType* InFiberObject);
	
	H1FiberHandleType* appConvertThreadToFiber(byte* InDataToFiber);
	bool appConvertFiberToThread();
	
	void appSwitchToFiber(H1FiberHandleType* InFiberObject);
	H1FiberHandleType* appGetCurrFiber();
	byte* appGetFiberData();
}
}