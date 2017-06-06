#include "H1PlatformThread.h"

using namespace SGD::Thread;

#include <process.h>

bool appCreateThread(CreateThreadOutput& Output, const CreateThreadInput& Input, H1ThreadEntryPoint ThreadEntryPoint, bool bResume)
{
	// create thread with suspended
	Output.ThreadHandle = CreateThread(nullptr, Input.StackSize, ThreadEntryPoint, nullptr, CREATE_SUSPENDED, (LPDWORD)&Output.ThreadId);

	if (Output.ThreadHandle == nullptr)
		return false;

	// resume the thread
	if (bResume)
	{
		ResumeThread(Output.ThreadHandle);
	}

	return true;
}

bool appDestroyThread(H1ThreadHandleType ThreadHandle)
{
	DWORD ExitCode;
	if (GetExitCodeThread(ThreadHandle, &ExitCode) == 0)
	{
		return false;
	}

	TerminateThread(ThreadHandle, ExitCode);
	return true;
}

void appJoinThreads(const JointThreadInput& Input)
{
	WaitForMultipleObjects(Input.NumThreads, Input.ThreadArray, true, INFINITE);
}

H1ThreadIdType appGetCurrentThreadId()
{
	return GetCurrentThreadId();
}

void appSetThreadAffinity(H1ThreadHandleType ThreadHandle, uint32 CPUCoreId)
{
	DWORD_PTR Mask = 1ull << CPUCoreId;
	SetThreadAffinityMask(ThreadHandle, Mask);
}