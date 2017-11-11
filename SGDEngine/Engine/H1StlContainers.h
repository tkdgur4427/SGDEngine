#pragma once

#include <vector>
#include <unordered_map>

namespace SGD
{
namespace Container
{
	template <class Type, class Allocator = SGD::Memory::H1StdAllocatorDefault<Type> >
	using H1Array = std::vector<Type, Allocator>;

	template <class KeyType, class ValueType, class Allocator = SGD::Memory::H1StdAllocatorDefault<Type> >
	using H1HashTable = std::unordered_map<KeyType, ValueType, Allocator>;
}
}