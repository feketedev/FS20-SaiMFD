#pragma once

#include <type_traits>
#include <new>



namespace Utils
{
	namespace Internal {
		
		template <class...Ts>
		struct TypeList {};

		template <class E, class... List>
		struct IsSingleElem {
			static constexpr bool value = std::is_same_v<TypeList<E>, TypeList<List...>>;
		};
	}



	template <class T>
	class Reassignable {
		union {
			T value;
		};

	public:
		T*		 Ptr()		   noexcept		{ return std::launder(&value); }
		const T* Ptr()	 const noexcept		{ return std::launder(&value); }

		T&		 Value()	   noexcept		{ return *Ptr(); }
		const T& Value() const noexcept		{ return *Ptr(); }


		Reassignable(const Reassignable& src)	noexcept (noexcept(T { src.value }))
			: value { src.value }
		{
		}

		Reassignable(Reassignable&& src)		noexcept (noexcept(T { std::move(src.value) }))
			: value { std::move(src.value) }
		{
		}

		template <class... Args, 
				  class = std::enable_if_t<!Internal::IsSingleElem<Reassignable<T>, std::decay_t<Args>...>::value>>
		Reassignable(Args&&... args)			noexcept (noexcept(T { std::forward<Args>(args)... }))
			: value { std::forward<Args>(args)... }
		{
		}

		~Reassignable()	 { value.~T(); }


		operator	   T&()			  noexcept 	{ return Value(); }
		operator const T&()		const noexcept	{ return Value(); }

		T&		 operator*()		  noexcept 	{ return Value(); }
		const T& operator*()	const noexcept	{ return Value(); }

		T*		 operator->()		  noexcept	{ return Ptr(); }
		const T* operator->()	const noexcept	{ return Ptr(); }


		Reassignable& operator=(const Reassignable& src) noexcept (noexcept(T { src.value }))
		{
			if (this != &src)
			{
				value.~T();
				new (&value) T { src.value };
			}
			return *this;
		}


		Reassignable& operator=(Reassignable&& src)		 noexcept (noexcept(T { std::move(src.value) }))
		{
			if (this != &src)
			{
				value.~T();
				new (&value) T { std::move(src.value) };
			}
			return *this;
		}
	};


}	// namespace Utils