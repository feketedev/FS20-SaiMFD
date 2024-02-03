#pragma once

#include <algorithm>
#include <type_traits>



namespace Utils
{
	template<class T>
	constexpr bool IsBoolEvaluable = std::is_convertible<T, bool>::value;



	// ----------------------------------------------------------------------------------

	template <class T>
	struct ArraySection {
		T* const		firstElem;
		const size_t	size;

		T* begin() const	{ return size ? firstElem : nullptr; }
		T* end()   const	{ return size ? firstElem + size : nullptr; }

		T& operator[](size_t i) const	{ return firstElem[i]; }
	};



	template <class T>
	struct OnExitAssignment {
		T&	data;
		T	finalValue;
		
		~OnExitAssignment()	noexcept(std::is_nothrow_move_assignable_v<T>)
		{ 
			data = std::move(finalValue); 
		}
	};

	template<class T, class Q>
	OnExitAssignment(T&, Q) -> OnExitAssignment<T>;




	// ---- Arithmetics -----------------------------------------------------------------


	template <unsigned P>
	struct Pow10;

	template <>
	struct Pow10<0> {
		static constexpr double value = 1.0;
	};

	template <unsigned P>
	struct Pow10 {
		static constexpr double value = Pow10<P - 1>::value * 10.0;
	};



	template <class N>
	N SubtractTillZero(N minued, N sub)
	{
		return minued >= sub ? minued - sub : 0;
	}


	template <class N>
	N Min(const N& first, const N& second)
	{
		return std::min<N>(first, second);
	}


	template <class N, class... More>
	N Min(const N& first, const N& second, const More&... more)
	{
		return std::min<N>(first, Utils::Min<N>(second, more...));
	}

	
	template <class N>
	N Max(const N& first, const N& second)
	{
		return std::max<N>(first, second);
	}


	template <class N, class... More>
	N Max(const N& first, const N& second, const More&... more)
	{
		return std::max<N>(first, Utils::Max<N>(second, more...));
	}




	// ---- For Containers --------------------------------------------------------------

	template<class Trg, class Src>
	void Append(Trg& target, Src&& source)
	{
		using S = std::remove_reference_t<Src>;
		using E = std::conditional_t<std::is_lvalue_reference_v<Src>,
									 decltype(*source.begin()),
									 typename S::value_type			>;
		for (E& el : source)
			target.push_back(std::forward<E>(el));
	}


	template<class C, class E>
	auto Find(C& container, const E& elem)
		-> std::enable_if_t<IsBoolEvaluable<decltype(*container.begin() == elem)>,
							decltype(container.begin())							 >
	{
		return std::find(container.begin(), container.end(), elem);
	}


	template<class C, class P>
	auto FindIf(C& container, P&& predicate)
		-> std::enable_if_t<IsBoolEvaluable<decltype(predicate(*container.begin()))>,
							decltype(container.begin())								>
	{
		return std::find_if(container.begin(), container.end(), predicate);
	}


	template<class C, class E>
	auto Contains(C& container, const E& elem)
		-> std::enable_if_t<IsBoolEvaluable<decltype(*container.begin() == elem)>, bool>
	{
		return Find(container, elem) != container.end();
	}


	template<class C, class P>
	auto AnyOf(C& container, P&& predicate)
		-> std::enable_if_t<IsBoolEvaluable<decltype(predicate(*container.begin()))>, bool>
	{
		return FindIf(container, predicate) != container.end();
	}


	template<class C, class P>
	auto AllOf(C& container, P&& predicate)
		-> std::enable_if_t<IsBoolEvaluable<decltype(predicate(*container.begin()))>, bool>
	{
		return FindIf(container, [&](auto& e) { return !predicate(e); }) == container.end();
	}


	/// @returns   Found any element to delete.
	template<class C, class E>
	auto EraseAllEqual(C& container, const E& elem)
		-> std::enable_if_t<IsBoolEvaluable<decltype(*container.begin() == elem)>, bool>
	{
		bool any = false;
		auto it = container.begin();
		while (it != container.end())
			if (any = (*it == elem))
				it = container.erase(it);
			else
				++it;
		
		return any;
	}

}
