#pragma once

#include "H1GlobalSingleton.h"
#include "H1Logger.h"

// only for memory use formatted debugf and checkf
#if !FINAL_RELEASE

// memory debugf
// debugf with message
#define h1MemDebug h1Debug
// debugf with formatted message
#define h1MemDebugf h1Debugf
// memory checkf
// checkf with message
#define h1MemCheck h1Check
// checkf with formatted message
#define h1MemCheckf h1Checkf

#else

// disable debugf and checkf for final release mode
#define h1MemDebug(message) __noop
#define h1MemDebugf(format, ...) __noop
#define h1MemCheck(condition, message) __noop
#define h1MemCheckf(condition, format, ...) __noop

#endif