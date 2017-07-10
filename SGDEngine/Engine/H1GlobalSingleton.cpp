#include "H1EnginePrivate.h"
#include "H1GlobalSingleton.h"

#include "H1MemoryArena.h"
#include "H1AsyncLogger.h"

SGD::Memory::H1MemoryArena* H1GlobalSingleton::MemoryArena()
{
	static SGD::Memory::H1MemoryArena MemoryArenaSingleton;
	return &MemoryArenaSingleton;
}

SGD::Log::H1AsyncLoggerAdmin* H1GlobalSingleton::AsyncLoggerAdmin()
{
	static SGD::Log::H1AsyncLoggerAdmin AsyncLoggerAdminSingleton;
	return &AsyncLoggerAdminSingleton;
}

