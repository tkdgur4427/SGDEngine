#pragma once

namespace SGD
{
namespace Thread
{
	// defining the layout (common interface of concurrent node)
	template <typename DataType>
	class H1ConcurrentNode
	{
	public:
		enum 
		{ 
			DataSize = sizeof(DataType), // the real data type size
		};

		struct Layout
		{
			DataType Data;
		};

		union
		{

		};
	};

	class H1ConcurrentNodeFactory
	{

	};
}
}