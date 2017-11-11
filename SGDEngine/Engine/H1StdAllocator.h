#pragma once

namespace SGD
{
namespace Memory
{
	// default std allocator 
	template <typename T>
	struct H1StdAllocatorDefault
	{
		// used for std::allocator_traits
		using value_type = T;

		explicit H1StdAllocatorDefault(size_t size)
		{

		}

		// needed for std::allocator_traits
		H1StdAllocatorDefault()
		{

		}

		// needed for std:;allocator_traits
		template <typename U>
		H1StdAllocatorDefault(const H1StdAllocatorDefault<U>& InAllocator)
		{

		}

		~H1StdAllocatorDefault()
		{

		}

		// needed for std::allocator_traits
		T* allocate(size_t InSize)
		{
			T* Pointer = static_cast<T*>(malloc(InSize * sizeof(T)));
			if (Pointer == nullptr && InSize > 0)
			{
				throw std::bad_alloc();
			}

			return Pointer;
		}

		// needed for std::allocator_traits
		void deallocate(T* InPointer, size_t)
		{
			free(InPointer);
		}

		template <typename U>
		struct rebind
		{
			typedef H1StdAllocatorDefault<U> other;
		};
	};

	// needed for std::allocator_traits
	template<typename T, typename U>
	constexpr bool operator!=(const H1StdAllocatorDefault<T>& a, const H1StdAllocatorDefault<U>& b) noexcept
	{
		// just naive implementation
		return &a != &b;
	}
}
}