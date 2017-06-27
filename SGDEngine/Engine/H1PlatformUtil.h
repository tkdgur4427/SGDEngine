namespace SGD {
namespace Platform {
namespace Util
{
		// bit operations
		bool appBitScanReverse(uint32& Offset, uint32 Mask);
		bool appBitScanReverse64(uint32& Offset, uint64 Mask);
		bool appBitScanForward(uint32& Offset, uint32 Mask);
		bool appBitScanForward64(uint32& Offset, uint64 Mask);
		// bit test and reset it to 0
		bool appBitTestAndReset(uint32 Offset, uint32& Mask);
		bool appBitTestAndReset64(uint32 Offset, uint64& Mask);
		bool appBitTestAndSet(uint32 Offset, uint32& Mask);
		bool appBitTestAndSet64(uint32 Offset, uint64& Mask);
		bool appBitTest(uint32 Offset, uint32 Mask);
		bool appBitTest64(uint32 Offset, uint64 Mask);

		// output debug string
		void appOutputDebugString(const char* String);

		// memory operation
		void appMemzero(byte* Address, int64 Size);
		void appMemcpy(byte* SrcAddress, byte* DestAddress, int64 Size);

		// string
		uint32 appStrLen(const char* Str);
		void appStrcpy(const char* Src, char* Dest);
}
}
}