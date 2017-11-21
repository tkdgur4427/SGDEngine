#include "H1PlatformThread.h"

using namespace SGD::Thread;

#include <process.h>

bool SGD::Thread::appCreateThread(CreateThreadOutput& Output, const CreateThreadInput& Input, H1ThreadEntryPoint ThreadEntryPoint, bool bResume)
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

bool SGD::Thread::appDestroyThread(H1ThreadHandleType ThreadHandle)
{
	DWORD ExitCode;
	if (GetExitCodeThread(ThreadHandle, &ExitCode) == 0)
	{
		return false;
	}

	TerminateThread(ThreadHandle, ExitCode);
	return true;
}

void SGD::Thread::appJoinThreads(const JointThreadInput& Input)
{
	WaitForMultipleObjects(Input.NumThreads, Input.ThreadArray, true, INFINITE);
}

H1ThreadIdType SGD::Thread::appGetCurrentThreadId()
{
	return GetCurrentThreadId();
}

void SGD::Thread::appSetThreadAffinity(H1ThreadHandleType ThreadHandle, uint32 CPUCoreId)
{
	DWORD_PTR Mask = 1ull << CPUCoreId;
	SetThreadAffinityMask(ThreadHandle, Mask);
}

void SGD::Thread::appSleep(int32 MilliSeconds)
{
	Sleep(MilliSeconds);
}

int32 SGD::Thread::appInterlockedCompareExchange32(volatile int32* Dest, int32 Exchange, int32 Comperand)
{
	return InterlockedCompareExchange((volatile LONG*)Dest, (LONG)Exchange, (LONG)Comperand);
}

int64 SGD::Thread::appInterlockedCompareExchange64(volatile int64* Dest, int64 Exchange, int64 Comperand)
{
	return InterlockedCompareExchange64(Dest, Exchange, Comperand);
}

H1FiberHandleType* SGD::Thread::appCreateFiber(int32 InStackSize, H1FiberEntryPoint InFiberEntryPoint, byte* InData)
{
	return (H1FiberHandleType*)CreateFiber(InStackSize, InFiberEntryPoint, InData);
}

void SGD::Thread::appDeleteFiber(H1FiberHandleType* InFiberObject)
{
	return DeleteFiber(InFiberObject);
}

H1FiberHandleType* SGD::Thread::appConvertThreadToFiber(byte* InDataToFiber)
{
	return (H1FiberHandleType*)ConvertThreadToFiberEx(InDataToFiber, FIBER_FLAG_FLOAT_SWITCH);
}

bool SGD::Thread::appConvertFiberToThread()
{
	return ConvertFiberToThread();
}

void SGD::Thread::appSwitchToFiber(H1FiberHandleType* InFiberObject)
{
	SwitchToFiber(InFiberObject);
}

H1FiberHandleType* SGD::Thread::appGetCurrFiber()
{
	return (H1FiberHandleType*)GetCurrentFiber();
}

byte* SGD::Thread::appGetFiberData()
{
	return (byte*)GetFiberData();
}