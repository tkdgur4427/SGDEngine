#pragma once

#include "H1MemoryArena.h"

#if SGD_USE_STD_ALLOCATOR
// for now, only overriding default allocator
#include "H1StdAllocator.h"
#endif