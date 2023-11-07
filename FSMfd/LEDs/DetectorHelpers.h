#pragma once

#include "SimVarDef.h"
#include "SimClient/IReceiver.h"
#include <array>
#include <type_traits>



namespace FSMfd::Led {

	// === This files contains some sugar for prettier construction of StateDetectors ===


	// ---- DiscreteDetector sugar ------------------------------------------------------

	template<class T, size_t N>
	using Triggers = std::array<T, N>;



	// ---- RangeDetector sugar ---------------------------------------------------------

	template<class T>
	struct Limit {
		const T		val;
		const bool	includedAbove;

		Limit (T v, bool ab = true) : val { v }, includedAbove { ab } {}

		bool MetBy(const T& p) const
		{
			return p < val || !includedAbove && p == val;
		}

		bool ExceededBy(const T& p) const
		{
			return val < p || includedAbove && p == val;
		}
	};

	template<class T>
	Limit<T> From(T v) { return { v, true }; }

	template<class T>
	Limit<T> UpIncluding(T v) { return { v, false }; }



	template<class T, size_t N>
	struct Limits : public std::array<Limit<T>, N>
	{
	};

	template <class T, class... Args>
	Limits(Limit<T>, Args...) -> Limits<T, sizeof...(Args) + 1>;

	template <class T, class... Args>
	Limits(T, Args...) -> Limits<T, sizeof...(Args) + 1>;



	// ---- Common: RequestType deduction -----------------------------------------------

	template <class T>
	struct RequestTypeFor {
		using SimvarValue = SimClient::SimvarValue;

		static constexpr RequestType value = std::is_floating_point_v<T> ? RequestType::Real
										   : std::is_signed_v<T>		 ? RequestType::SignedInt
										   : std::is_unsigned_v<T>		 ? RequestType::UnsignedInt
										   : RequestType::String;


	private:
		template <class TT>
		static constexpr auto GetAccessor(std::enable_if_t<std::is_floating_point_v<TT>>* = nullptr)
		{
			return &SimvarValue::AsDouble;
		}

		template <class TT>
		static constexpr auto GetAccessor(std::enable_if_t<std::is_integral_v<TT> && std::is_signed_v<TT>>* = nullptr)
		{
			return &SimvarValue::AsInt64;
		}

		template <class TT>
		static constexpr auto GetAccessor(std::enable_if_t<std::is_unsigned_v<TT>>* = nullptr)
		{
			return &SimvarValue::AsUnsigned32;
		}


		// TODO: Pointer? std::string?
		template <class TT>
		static constexpr auto GetAccessor(std::enable_if_t<std::is_pointer_v<TT>>* = nullptr)
		{
			return &SimvarValue::AsString;
		}
	public:
		static constexpr auto accessor = GetAccessor<T>();


		static_assert (std::is_arithmetic_v<T> || std::is_same_v<char*, std::decay_t<T>>, "Unsupported type.");
	};


}	// namespace FSMfd::Led
