#pragma once

#include <memory>

namespace SGD
{
	template<class Type, class Deleter = std::default_delete<Type> >
	using unique_ptr = std::unique_ptr<Type, Deleter>;

	template<class T>
	using remove_reference = std::remove_reference<T>;

	template<class T, class... Args>
	unique_ptr<T> make_unique(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	unique_ptr<T> make_unique(int32 size)
	{
		std::make_unique<T>(size);
	}

#if (__cplusplus <= 201402L) // until c++14
	template<class T>
	typename remove_reference<T>::type&& move(T&& t) noexcept
	{
		return std::move(t);
	}
#elif(__cplusplus > 201402L)
	template<class T>
	constexpr typename remove_reference<T>::type&& move(T&& t) noexcept
	{
		return std::move(t);
	}
#endif
}
