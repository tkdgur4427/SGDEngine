#include "H1EnginePrivate.h"

// only for windows platform
#if SGD_WINDOWS_PLATFORM
#include "H1PlatformUtil.h"

// include headers for windows specific
#include <intrin.h>

namespace SGD {
namespace Platform {
namespace Util
{
bool appBitScanReverse(uint32& Offset, uint32 Mask)
{
	return _BitScanReverse((unsigned long*)&Offset, Mask) != 0;
}

bool appBitScanReverse64(uint64& Offset, uint64 Mask)
{
	return _BitScanReverse64((unsigned long*)&Offset, Mask) != 0;
}

bool appBitScanForward(uint32& Offset, uint32 Mask)
{
	return _BitScanForward((unsigned long*)&Offset, Mask) != 0;
}

bool appBitScanForward64(uint64& Offset, uint64 Mask)
{
	return _BitScanForward64((unsigned long*)&Offset, Mask) != 0;
}

bool appBitTestAndReset(uint32 Offset, uint32& Mask)
{
	return _bittestandreset((long*)&Mask, Offset);
}

bool appBitTestAndReset64(uint64 Offset, uint64& Mask)
{
	return _bittestandreset64((long long*)&Mask, Offset);
}

bool appBitTestAndSet(uint32 Offset, uint32& Mask)
{
	return _bittestandset((long*)&Mask, Offset);
}

bool appBitTestAndSet64(uint64 Offset, uint64& Mask)
{
	return _bittestandset64((long long*)&Mask, Offset);
}

bool appBitTest(uint32 Offset, uint32 Mask)
{
	return _bittest((const long*)&Mask, Offset);
}

bool appBitTest64(uint64 Offset, uint64 Mask)
{
	return _bittest64((const long long*)&Mask, Offset);
}

void appOutputDebugString(const char* String)
{
	OutputDebugString(String);
}

void appMemzero(byte* Address, int64 Size)
{
	memset(Address, 0, Size);
}

void appMemcpy(byte* SrcAddress, byte* DestAddress, int64 Size)
{
	memcpy(DestAddress, SrcAddress, Size);
}

uint32 appStrLen(const char* Str)
{
	return (uint32)strlen(Str);
}

void appStrcpy(const char* Src, char* Dest)
{
	//strcpy(Dest, Src);
}

}
}
}
#endif