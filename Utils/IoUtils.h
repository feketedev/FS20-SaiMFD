#pragma once

#include <iostream>
#include <type_traits>



namespace Utils
{

	template <class S>
	struct FormatFlagScope {
		S&								stream;
		const std::ios_base::fmtflags	origFlags;
		
		static_assert(std::is_convertible_v<S&, std::ios_base&>);


		FormatFlagScope(S& s) : 
			stream    { s },
			origFlags { s.flags() }
		{
		}


		template <class T>
		S& operator<< (T&& in)
		{
			return stream << in;
		}


		template <class T>
		S& operator>> (T&& out)
		{
			return stream >> out;
		}


		~FormatFlagScope()
		{
			stream.setf(origFlags);
		}
	};

}
