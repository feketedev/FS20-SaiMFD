#pragma once

#include "SimVarDef.h"
#include "SimClient/IReceiver.h"
#include <array>
#include <type_traits>



namespace FSMfd::Led
{
	/* This files contains some sugar for prettier construction of StateDetectors. *
	 * Implicit type deduction is in focus.										   */


	// ---- DiscreteDetector sugar ------------------------------------------------------

	template<class T, size_t N>
	struct TriggerValues : public std::array<T, N> {
	};

	template <class T, class... Args>
	TriggerValues(T, Args...) -> TriggerValues<T, sizeof...(Args) + 1>;



	// ---- RangeDetector sugar ---------------------------------------------------------

	template<class T>
	struct Boundary {
		const T		Value;
		const bool	IncludedAbove;

		Boundary (T v, bool ab = true) : Value { v }, IncludedAbove { ab } {}

		// Note: templated to avoid narrowing conversions
		template <class P>
		bool MetBy(const P& p) const
		{
			return p < Value || !IncludedAbove && p == Value;
		}

		template <class P>
		bool ExceededBy(const P& p) const
		{
			return Value < p || IncludedAbove && p == Value;
		}
	};

	template<class T>
	Boundary<T> From(T v) { return { v, true }; }

	template<class T>
	Boundary<T> UpIncluding(T v) { return { v, false }; }



	template<class T, size_t N>
	struct Boundaries : public std::array<Boundary<T>, N> {
	};

	template <class T, class... Args>
	Boundaries(Boundary<T>, Args...) -> Boundaries<T, sizeof...(Args) + 1>;

	template <class T, class... Args>
	Boundaries(T, Args...) -> Boundaries<T, sizeof...(Args) + 1>;




	// ---- Aggregator strategies -------------------------------------------------------

	namespace Strategies
	{
		struct Max {
			template <class... Nums>
			static auto Aggregate(Nums... ns) { return Utils::Max(ns...); }
		};


		struct Min {
			template <class... Nums>
			static auto Aggregate(Nums... ns) { return Utils::Min(ns...); }
		};
	}




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
