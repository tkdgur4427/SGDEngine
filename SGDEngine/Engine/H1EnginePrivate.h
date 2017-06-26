#pragma once

// base type declarations
typedef unsigned char byte;
typedef int int32;
typedef unsigned int uint32;
typedef long int int64;
typedef unsigned long int uint64;

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
