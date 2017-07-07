#pragma once

#include "H1PlatformThread.h"

#define MAGIC_NUMBER_SPIN_COUNT 4000

namespace SGD
{
namespace Thread
{
	/*
		motivated from unreal engine design
			- But, it is implemented as spin lock (allowing reentrant lock)
	*/

	class H1CriticalSection
	{
	public:
		H1CriticalSection()
			: OwnerThreadId(0), LockCount(0), SpinCount(0)
		{}

		~H1CriticalSection()
		{}

		void Lock()
		{
			H1ThreadIdType ThreadId = appGetCurrentThreadId();
			if (ThreadId == OwnerThreadId)
			{
				LockCount++;
				return;
			}

			// doing local spinning
			InternalLock();

			// update owner thread id and increment lock count
			OwnerThreadId = ThreadId;
			LockCount++;
		}

		void UnLock()
		{
			H1ThreadIdType ThreadId = appGetCurrentThreadId();
			if (ThreadId == OwnerThreadId)
			{
				// hanlding reentrant lock
				LockCount--;

				if (LockCount == 0)
				{					
					OwnerThreadId = 0;
					InternalUnLock();
				}
			}
			else
			{
				LockCount = 0;
				OwnerThreadId = 0;
				InternalUnLock();
			}

			InternalUnLock();
		}

	protected:
		void InternalLock()
		{
			// implementing local spinning
			while (true)
			{
				auto Result = appInterlockedCompareExchange64(&Mutex, 1, 0);
				if (Result == 0)
				{
					return;
				}

				SpinCount++;

				// if the spin count reach to the limit (magic number), sleep
				if (SpinCount == MAGIC_NUMBER_SPIN_COUNT)
				{
					appSleep();
					SpinCount = 0;
				}
			}
		}

		void InternalUnLock()
		{
			// reset the mutex
			Mutex = 0;
		}

		volatile int64 Mutex;
		H1ThreadIdType OwnerThreadId;

		int32 LockCount;
		int32 SpinCount;
	};

	class H1ScopeLock
	{
	public:
		H1ScopeLock(H1CriticalSection* InSyncObject)
			: SyncObject(InSyncObject)
		{
			SyncObject->Lock();
		}

		~H1ScopeLock()
		{
			SyncObject->UnLock();
		}

	protected:
		// intentionally delete assignment operator
		H1ScopeLock& operator=(H1ScopeLock) = delete;

		H1CriticalSection* SyncObject;
	};
}
}