#pragma once

#include "H1GlobalSingleton.h"
#include "H1Logger.h"

// only for memory use formatted debugf and checkf
#if !FINAL_RELEASE

// memory debugf
// debugf with message
#define h1MemDebug(message) SGD::Log::H1Log<char>::CreateLog(message);
// debugf with formatted message
#define h1MemDebugf(format, ...) SGD::Log::H1Log<char>::CreateFormattedLog(format, __VA_ARGS__);
// memory checkf
// checkf with message
#define h1MemCheck(condition, meessage) if(!(condition)) { SGD::Log::H1Log<char>::CreateLog(meessage); SGD::Log::H1Log<char>::ForceToDump(); __debugbreak(); }
// checkf with formatted message
#define h1MemCheckf(condition, format, ...) if(!(condition)) { SGD::Log::H1Log<char>::CreateFormattedLog(format, __VA_ARGS__); SGD::Log::H1Log<char>::ForceToDump(); __debugbreak(); }

#else

// disable debugf and checkf for final release mode
#define h1MemDebug(message) __noop
#define h1MemDebugf(format, ...) __noop
#define h1MemCheck(condition, meessage) __noop
#define h1MemCheckf(condition, format, ...) __noop

#endif