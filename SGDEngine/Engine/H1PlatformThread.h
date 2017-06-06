#pragma once

// thread-related types
typedef void* H1ThreadHandleType;
typedef uint32 H1ThreadIdType;

// windows platform (PC)
#if SGD_WINDOWS_PLATFORM
typedef DWORD(WINAPI *H1ThreadEntryPoint)(LPVOID lpThreadParameter);
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
}
}