#pragma once

namespace SGD
{
namespace Container
{
namespace SinglelyLinkedList
{
	class H1Node
	{
	public:
		H1Node()
			: Next(nullptr)
		{}
		virtual ~H1Node()
		{}

		H1Node* Next;
	};
}
}
}