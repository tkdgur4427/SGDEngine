namespace SGD {
namespace Platform {
namespace Util
{
		// bit operations
		bool BitScanReverse(uint32& Offset, uint32 Mask);
		bool BitScanReverse64(uint32& Offset, uint64 Mask);
		bool BitScanForward(uint32& Offset, uint32 Mask);
		bool BitScanForward64(uint32& Offset, uint64 Mask);
		// bit test and reset it to 0
		bool BitTestAndReset(uint32 Offset, uint32& Mask);
		bool BitTestAndReset64(uint32 Offset, uint64& Mask);
		bool BitTestAndSet(uint32 Offset, uint32& Mask);
		bool BitTestAndSet64(uint32 Offset, uint64& Mask);
		bool BitTest(uint32 Offset, uint32 Mask);
		bool BitTest64(uint32 Offset, uint64 Mask);
}
}
}