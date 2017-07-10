#pragma once

/*
	H1LaunchEngineLoop
		- main entrance for SGDEngine
		- managing entire logic in here as a game engine
*/

// extern functionality for entering main thread (same as H1WorkerThread_Impl)

extern void Init();

extern void Run();

extern void Destroy();