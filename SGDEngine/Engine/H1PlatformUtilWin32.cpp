#include "H1EnginePrivate.h"

// only for windows platform
#if _WIN32
#include "H1PlatformUtil.h"

// include headers for windows specific
#include <intrin.h>

using namespace SGD::Platform::Util;

bool BitScanReverse(uint32& Offset, uint32 Mask)
{
	return _BitScanReverse((unsigned long*)&Offset, Mask) != 0;
}

bool BitScanReverse64(uint32& Offset, uint64 Mask)
{
	return _BitScanReverse64((unsigned long*)&Offset, Mask) != 0;
}

bool BitScanForward(uint32& Offset, uint32 Mask)
{
	return _BitScanForward((unsigned long*)&Offset, Mask) != 0;
}

bool BitScanForward64(uint64& Offset, uint64 Mask)
{
	return _BitScanForward64((unsigned long*)&Offset, Mask) != 0;
}

bool BitTestAndReset(uint32 Offset, uint32& Mask)
{
	return _bittestandreset((long*)&Mask, Offset);
}

bool BitTestAndReset64(uint64 Offset, uint64& Mask)
{
	return _bittestandreset64((long long*)&Mask, Offset);
}

bool BitTestAndSet(uint32 Offset, uint32& Mask)
{
	return _bittestandset((long*)&Mask, Offset);
}

bool BitTestAndSet64(uint64 Offset, uint64& Mask)
{
	return _bittestandset64((long long*)&Mask, Offset);
}

bool BitTest(uint32 Offset, uint32 Mask)
{
	return _bittest((const long*)&Mask, Offset);
}

bool BitTest64(uint64 Offset, uint64 Mask)
{
	return _bittest64((const long long*)&Mask, Offset);
}

#endif