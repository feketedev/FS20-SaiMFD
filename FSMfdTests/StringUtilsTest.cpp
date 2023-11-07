#include "pch.h"
#include "CppUnitTest.h"

#include "Utils/StringUtils.h"

#include "Utils/LiteSharedLock.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Utils::String;


namespace FsMfdTests
{
	template<class N>
	static void AssertPlaceNum(N value, StringSection trg, Align aln, std::wstring_view expectDigits)
	{
		const std::wstring orig = trg.buffer;
		const size_t s = trg.pos;
		const size_t e = trg.pos + trg.length;

		Assert::IsTrue(PlaceNumber(value, trg, aln));
		
		size_t iDigit = 0;
		for (size_t i = 0; i < trg.buffer.length(); i++)
		{
			if (i < s || e <= i)
			{
				Assert::AreEqual(orig[i], trg.buffer[i]);
				continue;
			}

			bool onDigit = trg.buffer[i] == expectDigits[iDigit];
			bool fin	 = expectDigits[iDigit] == L'\0';
			
			Assert::IsTrue(onDigit || trg.buffer[i] == L' ');
			Assert::IsTrue(onDigit 
				|| iDigit == 0 && aln != Align::Left							// padding before
				|| fin && aln == Align::Left									// padding after
				|| fin && aln == Align::RightPreferringOneSpace && i + 1 == e);	// final space

			if (onDigit)
				iDigit++;
		}
		Assert::IsTrue(expectDigits[iDigit] == L'\0');
	}


	template<class N>
	static void AssertPlaceNum(N value, StringSection trg, std::wstring_view expectDigits)
	{
		for (Align aln : { Align::Left, Align::Right, Align::RightPreferringOneSpace })
		{
			if (trg.length >= expectDigits.length())
				AssertPlaceNum(value, trg, aln, expectDigits);
		//	else
		//		AssertErrorMark(value, trg, aln);
		}
	}


	TEST_CLASS(StringUtilsTest)
	{

	public:

		TEST_METHOD(PlaceNumber_Ints)
		{
			Utils::LiteSharedLock lk;
			Utils::Test();

			constexpr size_t FullLen = 8;

			const std::pair<int, std::wstring_view> testNums[] = {
				{ 0,		L"0"		},
				{ 1,		L"1"		},
				{ -1,		L"-1"		},
				{ 12,		L"12"		},
				{ -12,		L"-12"		},
				{ 123456,	L"123456"	},
				{ -123456,	L"-123456"	},
				{ 1234567,	L"1234567"	},
				{ -1234567, L"-1234567"	},
				{ 12345678, L"12345678" }
			};

			std::wstring target;
			for (size_t pos = 0; pos < 9u; pos++)
			{
				for (size_t len = 1; len <= FullLen - pos; len++)
				{
					StringSection sect { target, pos, len };

					for (auto& [n, expect] : testNums)
					{
						if (expect.length() > len)
							break;

						target.assign(FullLen, L'X');
						AssertPlaceNum(n, sect, expect);
					}
				}
			}
		}
	};
}