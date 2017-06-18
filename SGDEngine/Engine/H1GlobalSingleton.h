#pragma once

// global singleton
//	- concept from the ppt from UbiSoft

class SGD::Memory::H1MemoryArena;

class H1GlobalSingleton
{
public:
	static SGD::Memory::H1MemoryArena* MemoryArena();
};