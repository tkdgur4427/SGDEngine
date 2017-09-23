#pragma once

/*
	H1LaunchEngineLoop
		- main entrance for SGDEngine
		- managing entire logic in here as a game engine
*/

// global variables
namespace SGD
{
	class H1LaunchEngineLoopGlobal
	{
	public:
		H1LaunchEngineLoopGlobal()
			: FrameNumber(0)
			, FrameNumberRenderThread(0)
		{}

		uint64 FrameNumber;
		uint64 FrameNumberRenderThread;
	};

}

// extern functionality for entering main thread (same as H1WorkerThread_Impl)

extern void Init();

extern void Run();

extern void Destroy();