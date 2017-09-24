#pragma once

namespace SGD {
namespace Platform {
namespace Util
{
		// bit operations
		bool appBitScanReverse(uint32& Offset, uint32 Mask);
		bool appBitScanReverse64(uint64& Offset, uint64 Mask);
		bool appBitScanForward(uint32& Offset, uint32 Mask);
		bool appBitScanForward64(uint64& Offset, uint64 Mask);
		// bit test and reset it to 0
		bool appBitTestAndReset(uint32 Offset, uint32& Mask);
		bool appBitTestAndReset64(uint64 Offset, uint64& Mask);
		bool appBitTestAndSet(uint32 Offset, uint32& Mask);
		bool appBitTestAndSet64(uint64 Offset, uint64& Mask);
		bool appBitTest(uint32 Offset, uint32 Mask);
		bool appBitTest64(uint64 Offset, uint64 Mask);

		// output debug string
		void appOutputDebugString(const char* String);

		// memory operation
		void appMemzero(byte* Address, int64 Size);
		void appMemcpy(const byte* SrcAddress, byte* DestAddress, int64 Size);

		// string
		uint32 appStrLen(const char* Str);
		void appStrcpy(const char* Src, char* Dest);

		// alignment

		// align a value to the nearest higher multiple of 'alignment', which must be a power of two
		template <typename T>
		constexpr inline T Align(const T Pointer, int64 Alignment)
		{
			return (T)(((int64)Pointer + Alignment - 1) & ~(Alignment - 1));
		}
}
}
}