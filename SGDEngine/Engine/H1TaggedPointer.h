#pragma once

namespace SGD
{
namespace Thread
{
	template <typename T>
	class H1TaggedPointer
	{
	public:
		

	protected:
		struct TaggedPointer
		{
			int64 Tag : 20;
			int64 Pointer : 44; // in windows, only 44 bits for address is allowed
		};

		union
		{
			TaggedPointer Layout;
			int64 Data;
		};
	};
}
}