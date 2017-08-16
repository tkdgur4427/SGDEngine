#pragma once

#if _WIN32
#define SGD_WINDOWS_PLATFORM 1

// windows-specific headers
#include <windows.h>
#endif

// final release flag
#define FINAL_RELEASE 0

// EASTL
#include "CorePrivate.h"

// static assert (compile-time)
#include "H1CompileTimeAssert.h"

// platform utilities
#include "H1PlatformUtil.h"

// memory
#include "H1MemoryArena.h"

// thread
#include "H1PlatformThread.h"
