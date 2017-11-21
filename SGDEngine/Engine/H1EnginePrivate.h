#pragma once

#if _WIN32
#define SGD_WINDOWS_PLATFORM 1

// windows-specific headers
#include <windows.h>
#endif

// final release flag
#define FINAL_RELEASE 0

#include "CorePrivate.h"

// static assert (compile-time)
#include "H1CompileTimeAssert.h"

// logger
#include "H1Logger.h"

// platform utilities
#include "H1PlatformUtil.h"

// using std allocator
#define SGD_USE_STD_ALLOCATOR 1

// memory
#include "H1Memory.h"

// thread
#include "H1PlatformThread.h"

// global singleton
#include "H1GlobalSingleton.h"

// by default, using stl container
#define SGD_USE_STL_CONTAINER 1

// check whether using stl container
#if SGD_USE_STL_CONTAINER
#include "H1StlContainers.h"
#endif