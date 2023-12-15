#pragma once

#include <type_traits>



namespace Utils::Cast
{

	// ----- For narrowing --------------------------------------------------------------

	/// Unchecked, but it would be irrational for this number not to fit in the destination type.
	template <class To, class From>
	constexpr To	Practically(From&& num) noexcept
	{
		using DT = std::decay_t<To>;
		using DF = std::decay_t<From>;

		static_assert(!std::is_lvalue_reference_v<To> || std::is_lvalue_reference_v<From>);
		static_assert(std::is_arithmetic_v<DT> && std::is_arithmetic_v<DF> &&
					  std::is_integral_v<DT> == std::is_integral_v<DF> &&
					  (  sizeof(To) < sizeof(From)
					  || std::is_signed_v<DT> != std::is_signed_v<DF>
					  ), "Not a narrowing conversion.");
		return static_cast<To>(num);
	}

	
	/// Narrowing the safety of which is ensured by class design or other checks in context.
	template <class To, class From>
	constexpr To	Implied(From&& num) noexcept
	{
		return Practically<To>(std::forward<From>(num));
	}




	// ----- For Enums ------------------------------------------------------------------

	/// Friendlier cast of enum classes to array index.
	template <class E>
	constexpr std::underlying_type_t<E>	AsIndex(E e) noexcept
	{
		static_assert(std::is_unsigned_v<std::underlying_type_t<E>>, "This enum can be negative!");
		return static_cast<std::underlying_type_t<E>>(e);
	}


}	// namespace Utils::Cast