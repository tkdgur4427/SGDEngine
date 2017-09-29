#pragma once

#include "H1AllocPolicy.h"

namespace SGD
{
namespace Memory
{
	template <class AllocPagePolicy = H1DefaultAllocPagePolicy>
	class H1LinearAllocPolicy : public H1AllocPolicy<AllocPagePolicy>
	{
		// not implemented yet!
		// ; the list of what we need to consider
		// 1. policy to use the free block (best-fit, large-block, ...)
		// 2. split and combine
	};
}
}