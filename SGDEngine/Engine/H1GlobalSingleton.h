#pragma once

// global singleton
//	- concept from the ppt from UbiSoft

namespace SGD
{
	class H1LaunchEngineLoopGlobal;

namespace Memory
{
	class H1MemoryArena;
}
namespace Log
{
	class H1AsyncLoggerAdmin;
}
}

class H1GlobalSingleton
{
public:
	static SGD::Memory::H1MemoryArena* MemoryArena();
	static SGD::Log::H1AsyncLoggerAdmin* AsyncLoggerAdmin();
	static SGD::H1LaunchEngineLoopGlobal* LaunchEngineLoopGlobal();
};