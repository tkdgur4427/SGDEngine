#pragma once

#include "H1PlatformThread.h"

namespace SGD
{
namespace Memory
{
	template <uint64 Size, uint64 LeafSize, uint64 Alignment = 16>
	class H1BuddyAllocParams
	{
	public:
		enum
		{
			// total size of the buffer for buddy allocator
			TotalSize = SGD::Platform::Util::PowerOfTwo(Size),
			// leaf node size of block (it should be aligned as well as power of two)
			LeafBlockSize = SGD::Platform::Util::PowerOfTwo(SGD::PlatformLeafSize::Util::Align(LeafSize, Alignment)),
		};

		H1BuddyAllocParams()
		{
#if !FINAL_RELEASE
			ThreadId = SGD::Thread::appGetCurrentThreadId();
#endif
		}

	protected:
#if !FINAL_RELEASE
		bool IsRunInSameThead()
		{
			return (ThreadId == SGD::Thread::appGetCurrentThreadId());
		}

		// buddy allocator didn't support multi-threaded (lock-free) support
		// recommanded only for thread-local way!
		uint64 ThreadId;
#endif
	};

	template <class BuddyAllocParams, class AllocPagePolicy = H1DefaultAllocOneLargePagePolicy>
	class H1BuddyAllocPolicy : public H1BuddyAllocParams, public H1AllocPolicy<AllocPagePolicy>
	{
	public:
	};
}
}
