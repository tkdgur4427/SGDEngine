#pragma once

#include "H1Logger.h"

namespace SGD
{
namespace Thread
{
	// lock free stack head (Lfs; Lock-free stack)
	template <class NodeType>
	class H1TaggedPointer
	{
	public:
		H1TaggedPointer()
			: Data(0)
		{
			h1Check(sizeof(TaggedPointer) == sizeof(int64), "tagged pointer's size should be same as int64")
		}

		// copying tag count as well as node pointer
		H1TaggedPointer(const H1TaggedPointer& InTaggedPointer)
		{
			Data = InTaggedPointer.Data;
		}

		~H1TaggedPointer()
		{}

		NodeType* GetNode()
		{
			return (NodeType*)Layout.Pointer;
		}

		// maintaining the tag count, change the node pointer
		void SetNode(NodeType* InNode)
		{
			h1Check((int64)InNode < (1 << 45), "node pointer range should be within [0, (1 << 45)]!");
			Layout.Pointer = (int64)InNode;
		}

		// increment tag count
		void IncrementTag()
		{
			Tag = Tag + 1;
			h1Check(Tag < (1 << 21), "Tag should be smaller than (1 << 21)!");
		}

		bool operator==(const H1TaggedPointer& InTaggedPointer)
		{
			return Data == InTaggedPointer.Data;
		}

		bool operator!=(const H1TaggedPointer& InTaggedPointer)
		{
			return !(*this == InTaggedPointer);
		}

	protected:
		struct TaggedPointer
		{
			int64 Tag : 20;
			int64 Pointer : 44; // in windows system, maximum allowed bits for address is 44 bits
		};

		union
		{
			TaggedPointer Layout;
			int64 Data;
		};
	};
}
}