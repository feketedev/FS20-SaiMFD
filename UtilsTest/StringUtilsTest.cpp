#include "CppUnitTest.h"

// #define  VERBOSE_TESTS
#include "TestUtils.h"

#include "Utils/StringUtils.h"
#include "Utils/DoubleShorthands.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Utils::String;


namespace FSMfdTests
{
#pragma region Exhaustive-test Helpers

	static void AssertDecimalTruncation(const std::wstring_view& fullText,
										const std::wstring_view& actual,
										const StringSection&	 target,
										size_t					 preferredPad)
	{
		const size_t period = fullText.find(DecimalSeparator);
		if (period == fullText.npos)
		{
			Assert::AreEqual(fullText.length(), actual.length());
			return;
		}
		Assert::AreEqual(period, fullText.rfind(DecimalSeparator));
		Assert::IsTrue(period <= actual.length(),	  L"Whole digits truncated");
		Assert::IsTrue(period + 1 != actual.length(), L"Truncated at decimal point");
		if (period < actual.length())
		{
			Assert::AreEqual(DecimalSeparator, actual[period]);
		}
		else
		{
			const size_t space = target.Length - actual.length();
			Assert::IsTrue(space < 2 + preferredPad, L"Unnecessary truncation.");
		}
	}

	enum Truncability { Forbidden, Regulated, Free };

	static auto AssertAlignment(const std::wstring_view& expText,
								const StringSection&	 actTarget,
								const PaddedAlignment&	 aln,
								Truncability			 trunc	  )
								-> std::wstring_view	 // found text without alignment
	{ 
		Assert::IsTrue(actTarget.Length > 0);

		const wchar_t* trg = actTarget.GetStart();

		size_t left = 0;
		while (left < actTarget.Length && trg[left] == L' ')
			++left;

		size_t right = actTarget.Length;
		while (0 < right && trg[right - 1] == L' ')
			--right;

		// -- check copied text --
		Assert::IsTrue(left < right);

		const size_t copied = right - left;
		Assert::IsTrue(copied <= expText.length());
		for (size_t i = 0; i < copied; i++)
			Assert::AreEqual(expText[i], trg[left + i], L"Copied text differs.");

		Assert::IsTrue(copied <= expText.length());
		Assert::IsTrue(trunc != Forbidden || copied == expText.length(), L"Copied text got truncated. It was forbidden.");
		
		// -- check padding --
		const size_t rspace = actTarget.Length - right;

		const auto [padding, alignment] = (aln.direction == Align::Right)
										   ? std::make_pair(rspace, left)
										   : std::make_pair(left, rspace);

		Assert::IsTrue(padding <= aln.pad,										L"Unrequested padding.");
		Assert::IsTrue(padding == aln.pad || trunc != Free && alignment == 0,	L"Unnecessary decrease of requested padding");
		Assert::IsTrue(trunc != Free || padding == aln.pad,						L"Reduced padding. It was mandatory.");
		Assert::IsTrue(copied == expText.length() 
					|| copied + alignment < expText.length(),					L"Unnecessaryly truncated text.");

		return { trg + left, copied };
	}



	// buffer left intact outside targeted StringSection
	static void AssertIntactContext(wchar_t filler, const StringSection& trg)
	{
		const size_t s = trg.Pos;
		const size_t e = trg.Pos + trg.Length;

		for (size_t i = 0; i < s; i++)
			Assert::AreEqual(filler, trg.buffer[i]);

		for (size_t i = e; i < trg.buffer.length(); i++)
			Assert::AreEqual(filler, trg.buffer[i]);
	}


	static void AssertErrorString(const StringSection& trg, const PaddedAlignment& aln)
	{
		AssertAlignment(L"##", trg, aln, Truncability::Free);
	}



	template <class N>
	static bool IsPlaceable(const std::wstring_view& fullNumber, size_t spaceAvail)
	{
		return fullNumber.length() <= spaceAvail;
	}

	template <>
	static bool IsPlaceable<double>(const std::wstring_view& fullNumber, size_t spaceAvail)
	{
		size_t period = fullNumber.find(DecimalSeparator);

		bool   hasPeriod   = period != fullNumber.npos;
		size_t wholeDigits = hasPeriod ? period : fullNumber.length();

		return wholeDigits <= spaceAvail;
	}


	template <class N, class... Args>
	void AssertPlaceNum(N value, StringSection trg, std::wstring_view expectDigits, Args&&... placeExtras)
	{
		const size_t fullLen = trg.buffer.length();

		for (Align alnDir : { Align::Left, Align::Right })
		{
			for (unsigned pad : { 0, 1, 2 })
			{
				if (trg.Length <= pad)				// NOTE: this is asserted by PlaceNumber
					break;

				PaddedAlignment aln { alnDir, pad };

				trg.buffer.assign(fullLen, L'X');

				const bool shouldSucceed = IsPlaceable<N>(expectDigits, trg.Length);
				Assert::AreEqual(shouldSucceed, PlaceNumber(value, placeExtras..., trg, aln));
			
				VERBOSE ('\t', trg.buffer.data(), L"    \tpad: ", aln.pad, L"\n");


				if (shouldSucceed)
				{
					constexpr bool isFloat = std::is_floating_point<N>::value;
					auto copiedPart = AssertAlignment(expectDigits, trg, aln, isFloat ? Truncability::Regulated : Truncability::Forbidden);
					if (isFloat)
						AssertDecimalTruncation(expectDigits, copiedPart, trg, aln.pad);
				}
				else
				{
					AssertErrorString(trg, aln);
				}

				AssertIntactContext(L'X', trg);
				
				if (!shouldSucceed)
					break;
			}
		}
	}


	template <class Action>
	void ForAllSectionsIn(std::wstring& str, Action&& act)
	{
		const size_t fullLen = str.length();
		for (size_t pos = 0; pos <= fullLen; pos++)
		{
			for (size_t len = 1; len <= fullLen - pos; len++)
			{
				StringSection sect { str, pos, len };
				act(sect);
			}
		}
	}

#pragma endregion



	TEST_CLASS(StringUtilsTest)
	{
	public:

		TEST_METHOD (PlaceNumber_Ints)
		{
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
				{ 12345678, L"12345678" },
			};

			std::wstring target (FullLen, L'X');
			ForAllSectionsIn(target, [&](StringSection sect)
			{
				for (auto& [n, expected] : testNums)
				{
					// check overrun by 1, 2 - above that is unnecessary
					if (expected.length() > sect.Length + 2)
						break;

					VERBOSE (expected.data(), '\n');
					AssertPlaceNum(n, sect, expected);
				}
			});
			
		}


		TEST_METHOD (PlaceNumber_Ints_Basic)
		{
			std::wstring buff(5, L' ');
			const StringSection trg { buff, 0 };

			Assert::IsTrue(PlaceNumber(1, trg, Align::Right));
			Assert::AreEqual(L"    1", buff.data());
			Assert::IsTrue(PlaceNumber(1, trg, Align::Left));
			Assert::AreEqual(L"1    ", buff.data());

			Assert::IsTrue(PlaceNumber(1, trg, { Align::Right, 1 }));
			Assert::AreEqual(L"   1 ", buff.data());
			Assert::IsTrue(PlaceNumber(1, trg, { Align::Left, 1 }));
			Assert::AreEqual(L" 1   ", buff.data());
			Assert::IsTrue(PlaceNumber(12, trg, { Align::Right, 1 }));
			Assert::AreEqual(L"  12 ", buff.data());
			Assert::IsTrue(PlaceNumber(12, trg, { Align::Left, 1 }));
			Assert::AreEqual(L" 12  ", buff.data());
			Assert::IsTrue(PlaceNumber(12, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"12   ", buff.data());
			Assert::IsTrue(PlaceNumber(12, trg, { Align::Left, 3 }));
			Assert::AreEqual(L"   12", buff.data());
			Assert::IsTrue(PlaceNumber(-12, trg, { Align::Right, 2 }));
			Assert::AreEqual(L"-12  ", buff.data());
			Assert::IsTrue(PlaceNumber(-12, trg, { Align::Left, 2 }));
			Assert::AreEqual(L"  -12", buff.data());

			// padding reduced if needed
			Assert::IsTrue(PlaceNumber(123, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"123  ", buff.data());
			Assert::IsTrue(PlaceNumber(123, trg, { Align::Left, 3 }));
			Assert::AreEqual(L"  123", buff.data());

			// insufficient room for number
			Assert::IsFalse(PlaceNumber(123456, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"##   ", buff.data());
			Assert::IsFalse(PlaceNumber(123456, trg, { Align::Left, 3 }));
			Assert::AreEqual(L"   ##", buff.data());
			Assert::IsFalse(PlaceNumber(-12345, trg, { Align::Left, 3 }));
			Assert::AreEqual(L"   ##", buff.data());
		}


		TEST_METHOD (PlaceNumber_Decimals_Basic)
		{
			std::wstring buff(10, L' ');
			const StringSection trg { buff, 0 };

			Assert::IsTrue(PlaceNumber(1.046875, 6, trg, Align::Right));
			Assert::AreEqual(L"  1.046875", buff.data());

			// Truncate always - no IEEE 754 Bankers' rounding
			// (I think in our context Mach 0.95 shouldn't be indicated as Mach 1.0)
			Assert::IsTrue(PlaceNumber(1.046875, 5, trg, Align::Right));
			Assert::AreEqual(L"   1.04687", buff.data());

			// No traditional rounding either
			Assert::IsTrue(PlaceNumber(1.046875, 3, trg, Align::Right));
			Assert::AreEqual(L"     1.046", buff.data());

			// Requested zeroes are displayed
			Assert::IsTrue(PlaceNumber(1.046875, 1, trg, Align::Right));
			Assert::AreEqual(L"       1.0", buff.data());
			Assert::IsTrue(PlaceNumber(2.0, 2, trg, Align::Right));
			Assert::AreEqual(L"      2.00", buff.data());

			// Simple padding
			Assert::IsTrue(PlaceNumber(1.046875, 5, trg, { Align::Right, 1 }));
			Assert::AreEqual(L"  1.04687 ", buff.data());
			Assert::IsTrue(PlaceNumber(1.046875, 5, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"1.04687   ", buff.data());

			// Padding truncates decimals
			Assert::IsTrue(PlaceNumber(1.046875, 5, trg, { Align::Right, 4 }));
			Assert::AreEqual(L"1.0468    ", buff.data());
			Assert::IsTrue(PlaceNumber(1.046875, 5, trg, { Align::Right, 7 }));
			Assert::AreEqual(L"1.0       ", buff.data());

			// No truncation at separator
			Assert::IsTrue(PlaceNumber(1.046875, 5, trg, { Align::Right, 8 }));
			Assert::AreEqual(L" 1        ", buff.data());
			Assert::IsTrue(PlaceNumber(1.046875, 5, trg, { Align::Right, 9 }));
			Assert::AreEqual(L"1         ", buff.data());

			// No truncation of whole part - padding necessarily reduced
			// NOTE: a mandatory padding would be equivalent of using smaller trg.
			Assert::IsTrue(PlaceNumber(10.34567, 5, trg, { Align::Right, 9 }));
			Assert::AreEqual(L"10        ", buff.data());

			// Overrun symbol respects padding
			Assert::IsFalse(PlaceNumber(12345678901.1, 0, trg, { Align::Right, 1 }));
			Assert::AreEqual(L"       ## ", buff.data());
			Assert::IsFalse(PlaceNumber(12345678901.1, 0, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"     ##   ", buff.data());

			// Negative sign requires 1 extra space
			Assert::IsFalse(PlaceNumber(-1234567890.1, 0, trg, { Align::Right, 1 }));
			Assert::AreEqual(L"       ## ", buff.data());
			Assert::IsFalse(PlaceNumber(-1234567890.1, 0, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"     ##   ", buff.data());
			Assert::IsTrue(PlaceNumber(-123456789.1, 0, trg, { Align::Right, 1 }));
			Assert::AreEqual(L"-123456789", buff.data());
			Assert::IsTrue(PlaceNumber(-123456789.1, 0, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"-123456789", buff.data());
			Assert::IsTrue(PlaceNumber(-1234567.1, 0, trg, { Align::Right, 1 }));
			Assert::AreEqual(L" -1234567 ", buff.data());
			Assert::IsTrue(PlaceNumber(-1234567.1, 0, trg, { Align::Right, 3 }));
			Assert::AreEqual(L"-1234567  ", buff.data());

			// non-default overrun symbol
			Assert::IsFalse(PlaceNumber(12345678901.1, 0, trg, { Align::Right, 3 }, L"OVR"));
			Assert::AreEqual(L"    OVR   ", buff.data());

			// Left...
			Assert::IsTrue(PlaceNumber(1.046875, 6, trg, Align::Left));
			Assert::AreEqual(L"1.046875  ", buff.data());

			// With decimals preferred over padding
			Assert::IsTrue(PlaceNumber(1.046875, DecimalUsage { 4, /*preferDecimals:*/ true }, trg, { Align::Right, 8, }));
			Assert::AreEqual(L"1.0468    ", buff.data());
			Assert::IsTrue(PlaceNumber(1.046875, DecimalUsage { 4, /*preferDecimals:*/ true }, trg, { Align::Right, 2 }));
			Assert::AreEqual(L"  1.0468  ", buff.data());

			buff.assign(9, L' ');
			StringSection shortTrg = trg.SubSection(0, 3);
			
			Assert::IsTrue(PlaceNumber(1.04687,  DecimalUsage { 4, /*preferDecimals:*/ true }, shortTrg, { Align::Right, 1, }));
			Assert::AreEqual(L"1.0      ", buff.data());
			Assert::IsTrue(PlaceNumber(12.04687, DecimalUsage { 4, /*preferDecimals:*/ true }, shortTrg, { Align::Right, 1, }));
			Assert::AreEqual(L"12       ", buff.data());
			Assert::IsTrue(PlaceNumber(123.0468, DecimalUsage { 4, /*preferDecimals:*/ true }, shortTrg, { Align::Right, 1, }));
			Assert::AreEqual(L"123      ", buff.data());
			Assert::IsTrue(PlaceNumber(1.04687, DecimalUsage { 4, /*preferDecimals:*/ true }, shortTrg, Align::Right));
			Assert::AreEqual(L"1.0      ", buff.data());
			Assert::IsTrue(PlaceNumber(12.04687, DecimalUsage { 4, /*preferDecimals:*/ true }, shortTrg, Align::Right));
			Assert::AreEqual(L" 12      ", buff.data());
			Assert::IsTrue(PlaceNumber(123.0468, DecimalUsage { 4, /*preferDecimals:*/ true }, shortTrg, Align::Right));
			Assert::AreEqual(L"123      ", buff.data());
		}


		TEST_METHOD (PlaceNumber_Extremes)
		{
			constexpr uint8_t signif = std::numeric_limits<double>::digits10;

			const double minGuaranteed = std::nextafter(PlaceDoubleLowerLimit, +Inf);
			const double maxGuaranteed = std::nextafter(PlaceDoubleUpperLimit, -Inf);
			
			std::wstring  buff(3 * signif, L' ');
			StringSection trg { buff, 0 };

			Assert::IsTrue(PlaceNumber(minGuaranteed, signif, trg, Align::Left));
			Assert::IsTrue(PlaceNumber(maxGuaranteed, signif, trg, Align::Left));

			// Failure for numbers outside limits is not guaranteed by interface,
			// however let's run this branch too.
			Assert::IsFalse(PlaceNumber(PlaceDoubleLowerLimit, signif, trg, Align::Left));
			Assert::IsFalse(PlaceNumber(PlaceDoubleUpperLimit, signif, trg, Align::Left));
		}


		TEST_METHOD (PlaceNumber_Decimals)
		{
			constexpr size_t FullLen = 8;

			struct DecimalTestCase {
				double				value;
				std::wstring_view	expected;
				unsigned			precision = 4;
			};

			const DecimalTestCase testNums[] = {
				{ 0.0,		L"0.0000"	},
				{ -0.0,		L"-0.0000"	},
				{ 2.0,		L"2.0000"	},
				{ 2.0625,	L"2.0625"	},
				{ 2.0625,	L"2.06",	2	},
				{ 2.0625,	L"2.0",		1	},		// no round up; keep the 0
				{ 2.0625,	L"2",		0	},		// no trunc at decimal point
				{ 2.4625,	L"2",		0	},		// no trunc at decimal point
				{ -2.0,		L"-2.0000"	},
				{ -2.0625,	L"-2.0625"	},
				{ -2.0625,	L"-2.06",	2	},
				{ -2.0625,	L"-2.0",	1	},		// no round up; keep the 0
				{ -2.0625,	L"-2",		0	},		// no trunc at decimal point
			};

			std::wstring target (FullLen, L'X');
			ForAllSectionsIn(target, [&](StringSection sect)
			{
				VERBOSE ("\n--- Target: ", sect.Pos, " len ", sect.Length, " ---\n");

				for (auto& [n, expected, precision] : testNums)
				{
					VERBOSE ("\nTest num: ", expected.data(), " precision: ", precision, "\n");

					AssertPlaceNum(n, sect, expected, precision);
				}
			});
		}


		TEST_METHOD (PlaceNumber_SignOptions)
		{
			std::wstring  buff(5, L' ');
			StringSection trg { buff, 0 };

			Assert::IsTrue(PlaceNumber(1, trg));
			Assert::AreEqual(L"    1", buff.c_str());
			Assert::IsTrue(PlaceNumber(1, SignUsage::PrependPlus, trg));
			Assert::AreEqual(L"   +1", buff.c_str());
			Assert::IsTrue(PlaceNumber(1, SignUsage::PrependPlus | SignUsage::ForbidNegativeValues, trg));
			Assert::AreEqual(L"   +1", buff.c_str());
			Assert::IsTrue(PlaceNumber(1, SignUsage::ForbidNegativeValues, trg));
			Assert::AreEqual(L"    1", buff.c_str());


			Assert::IsTrue(PlaceNumber(5123, trg));
			Assert::AreEqual(L" 5123", buff.c_str());
			Assert::IsTrue(PlaceNumber(5123, SignUsage::PrependPlus, trg));
			Assert::AreEqual(L"+5123", buff.c_str());

			Assert::IsTrue(PlaceNumber(51234, trg));
			Assert::AreEqual(L"51234", buff.c_str());
			Assert::IsFalse(PlaceNumber(51234, SignUsage::PrependPlus, trg, Align::Right, L"n/a"));
			Assert::AreEqual(L"  n/a", buff.c_str());
			Assert::IsFalse(PlaceNumber(51234, SignUsage::PrependPlus | SignUsage::ForbidNegativeValues, trg, Align::Right, L"n/a"));
			Assert::AreEqual(L"  n/a", buff.c_str());
			Assert::IsTrue(PlaceNumber(51234, SignUsage::ForbidNegativeValues, trg));
			Assert::AreEqual(L"51234", buff.c_str());

			Assert::IsTrue(PlaceNumber(-1, trg));
			Assert::AreEqual(L"   -1", buff.c_str());
			Assert::IsTrue(PlaceNumber(1, SignUsage::PrependPlus, trg));
			Assert::AreEqual(L"   +1", buff.c_str());
			Assert::IsFalse(PlaceNumber(-1, SignUsage::PrependPlus | SignUsage::ForbidNegativeValues, trg, Align::Right, L"n/a"));
			Assert::AreEqual(L"  n/a", buff.c_str());
			Assert::IsFalse(PlaceNumber(-1, SignUsage::ForbidNegativeValues, trg, Align::Right, L"n/a"));
			Assert::AreEqual(L"  n/a", buff.c_str());


			// --- Decimals ---

			Assert::IsTrue(PlaceNumber(1.0, 2, trg));
			Assert::AreEqual(L" 1.00", buff.c_str());

			for (SignUsage negOption : { SignUsage::Default, SignUsage::ForbidNegativeValues })
			{
				Assert::IsTrue(PlaceNumber(1.0, { 2, SignUsage::PrependPlus | negOption }, trg));
				Assert::AreEqual(L"+1.00", buff.c_str());

				Assert::IsTrue(PlaceNumber(12.0, { 2, negOption }, trg));
				Assert::AreEqual(L"12.00", buff.c_str());
				Assert::IsTrue(PlaceNumber(12.0, { 2, SignUsage::PrependPlus | negOption }, trg));
				Assert::AreEqual(L"+12.0", buff.c_str());
				Assert::IsTrue(PlaceNumber(123.0, { 2, SignUsage::PrependPlus | negOption }, trg));
				Assert::AreEqual(L" +123", buff.c_str());

				Assert::IsTrue(PlaceNumber(0.0, { 2, negOption }, trg));
				Assert::AreEqual(L" 0.00", buff.c_str());
			}

			Assert::IsTrue(PlaceNumber(0.0, { 2, SignUsage::PrependPlus }, trg));
			Assert::AreEqual(L"+0.00", buff.c_str());

			Assert::IsFalse(PlaceNumber(-0.0, { 2, SignUsage::ForbidNegativeValues }, trg, Align::Right, L"n/a"));
			Assert::AreEqual(L"  n/a", buff.c_str());
		}
	};
}
