#include "H1EnginePrivate.h"
#include "H1GlobalSingleton.h"

#include "H1MemoryArena.h"

SGD::Memory::H1MemoryArena* H1GlobalSingleton::MemoryArena()
{
	static SGD::Memory::H1MemoryArena MemoryArenaSingleton;
	return &MemoryArenaSingleton;
}