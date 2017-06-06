#include <QtWidgets>

#ifndef _WIN32
#define _WIN32
#endif

// SGD libraries
#include "..\\Core\\CorePrivate.h"
#include "..\\Engine\\H1EnginePrivate.h"

// linking
#if DEBUG
#pragma comment(lib, "..\\x64\\Debug\\Core.lib")
#pragma comment(lib, "..\\x64\\Debug\\Engine.lib")
#endif
