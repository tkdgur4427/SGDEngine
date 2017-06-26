#pragma once

#include "H1Memory.h"

namespace SGD
{
namespace Memory
{		
	class H1StackAllocator
	{
	public:
		H1StackAllocator()
		{
			
		}

	protected:		
		byte* StartAddress;
		byte* EndAddress;
	};
}
}