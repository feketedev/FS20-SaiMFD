#pragma once

#ifdef VERBOSE_TESTS
#	define VERBOSE(...)		::FsMfdTests::Print(__VA_ARGS__)
#else
#	define VERBOSE(...)
#endif



namespace FsMfdTests
{
	void PrintVar(const wchar_t* s)	{ Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(s); }
	void PrintVar(const char*	  s)	{ Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(s); }
	
	template <class N>
	void PrintVar(N val, std::enable_if_t<std::is_floating_point_v<N>>* = nullptr)
	{
		char buff[std::numeric_limits<N>::digits10 * 2 + 2];
		Assert::IsTrue(0 < sprintf_s(buff, "%f", val));
		PrintVar(buff);
	}

	template <class N>
	void PrintVar(N val, std::enable_if_t<std::is_integral_v<N>>* = nullptr)
	{
		char buff[std::numeric_limits<N>::digits10 + 1];

		constexpr char* format =  sizeof(N) == sizeof(long long) ? "%lld"
								: sizeof(N) == sizeof(long)		 ? "%ld"
								: "%d";

		Assert::IsTrue(0 < sprintf_s(buff, format, val));
		PrintVar(buff);
	}

	template <>
	void PrintVar<bool>(bool val, std::enable_if_t<std::is_integral_v<bool>>*)
	{
		PrintVar(val ? "true" : "false");
	}

	template <>
	void PrintVar<char>(char val, std::enable_if_t<std::is_integral_v<char>>*)
	{
		char buff[2] = { val, '\0' };
		PrintVar(buff);
	}

	template <>
	void PrintVar<wchar_t>(wchar_t val, std::enable_if_t<std::is_integral_v<wchar_t>>*)
	{
		wchar_t buff[2] = { val, L'\0' };
		PrintVar(buff);
	}



	template <class... Args>
	void Print(const Args&... args)
	{
		auto write = [&](const auto& msg) -> bool	// simple hack to expand args easily
		{
			PrintVar(msg);
			return true;
		};

		auto _ress = { write(args)... };
	}
}