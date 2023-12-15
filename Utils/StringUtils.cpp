#include "StringUtils.h"

#include "BasicUtils.h"
#include "CastUtils.h"
#include "Debug.h"

#include <cfenv>
#include <locale>
#include <utility>


namespace Utils::String
{

#pragma region Consts+Locale

	constexpr wchar_t  Space = L' ';	 // clear character for alignments

	constexpr unsigned Int32MaxChars	 = std::numeric_limits<int32_t>::digits10 + 2;		// counteract round-rown + mind the '-'

	constexpr unsigned SignificantDigits = std::numeric_limits<double>::digits10;
	constexpr unsigned MaxFractionDigits = SignificantDigits;								// max fraction digits to write
	constexpr unsigned MaxWholeDigits	 = SignificantDigits;								// max whole part digits to write
	
	// over this length really should use scientific form - not implemented here
	constexpr unsigned PlaceDoubleMaxChars = MaxWholeDigits + MaxFractionDigits + 1;		// digits + '.', no extra for '-'

	
	// -- extern --
	constexpr double   PlaceDoubleUpperLimit = Utils::Pow10<MaxWholeDigits>::value;
	constexpr double   PlaceDoubleLowerLimit = -Utils::Pow10<MaxWholeDigits - 1>::value;	// mind the '-'

	
	// Read current locale for swprintf.
	static wchar_t GetDecimalSeparator()
	{
		auto* loc = localeconv();
		if (loc && loc->decimal_point)		// can be null or '\0' (?)
		{
			wchar_t res;
			if (mbtowc(&res, loc->decimal_point, 1) > 0)
				return res;
		}
		DBG_BREAK;
		return L'.';
	}

	// -- extern --
	const wchar_t DecimalSeparator = GetDecimalSeparator();

#pragma endregion



#pragma region StringSection

	StringSection::StringSection(std::wstring& buffer, size_t pos, size_t length) :
		buffer	{ buffer },
		Pos		{ pos },
		Length	{ length }
	{
		LOGIC_ASSERT (pos + length <= buffer.length());		// wstring excludes \0
	}


	StringSection::StringSection(std::wstring& buffer, size_t pos) :
		StringSection { buffer, pos, buffer.length() - pos }
	{
	}


	StringSection StringSection::FollowedBy(size_t nextLength) const
	{
		return { buffer, Pos + Length, nextLength };
	}


	StringSection StringSection::SubSection(size_t offset, size_t length) const
	{
		LOGIC_ASSERT (offset + length <= this->Length);

		return { buffer, Pos + offset, length };
	}

	
	wchar_t& StringSection::operator[](size_t i) const
	{
		DBG_ASSERT (i < Length);
		return buffer[Pos + i];
	}


	wchar_t* StringSection::GetStart() const
	{
		return buffer.data() + Pos;
	}


	wchar_t * StringSection::GetLast() const
	{
		LOGIC_ASSERT (Length > 0);
		return GetStart() + Length - 1;
	}


	std::wstring_view StringSection::AsStringView() const
	{
		return { GetStart(), Length };
	}


	void StringSection::FillIn(std::wstring_view src)
	{
		DBG_ASSERT	   (src.length() == Length);
		LOGIC_ASSERT_M (buffer.size() >= Pos + Length, "Buffer shortened in meantime?")

		wchar_t* trg = GetStart();
		for (size_t i = 0; i < src.length(); i++)
			trg[i] = src[i];
	}


	void StringSection::FillWith(wchar_t c)
	{
		wchar_t* trg = GetStart();
		for (size_t i = 0; i < Length; i++)
			trg[i] = c;
	}

#pragma endregion



#pragma region CoreHelpers

	// Calls the non-throwing swprintf.
	// The templated swprintf_s throws if buffer is too small.
	template <size_t S, class... Args>
	int PrintTo(wchar_t (&buff)[S], const wchar_t* format, const Args&... args)
	{
		return swprintf(buff, S, format, args...);
	}

#pragma endregion



#pragma region TextAlignments

	auto PaddedAlignment::CalcPosition(size_t text, size_t window, bool preferPadding) const -> Position
	{
		const size_t requested = text + pad;
		const size_t overrun   = SubtractTillZero(requested, window);

		Position res;
		{
			// ([in] request -> [out] achievable) pairs
			std::pair<const size_t&, size_t&> padCalc  { pad,  res.padRemain };
			std::pair<const size_t&, size_t&> textCalc { text, res.textLen   };

			// order by preference to truncate
			auto& primary   = preferPadding ? padCalc : textCalc;
			auto& secondary = preferPadding ? textCalc : padCalc;

			primary.second   = std::min(primary.first, window);
			secondary.second = SubtractTillZero(secondary.first, overrun);
		}
		DBG_ASSERT (res.textLen + res.padRemain <= window);
		DBG_ASSERT (!preferPadding || res.padRemain == pad);

		res.textStart = (direction == Align::Right)
			? window - res.padRemain - res.textLen
			: res.padRemain;

		return res;
	}


	static size_t FillTarget(const std::wstring_view& src, size_t off, size_t maxCount, StringSection target)
	{
		DBG_ASSERT (off + maxCount <= target.Length);	// expects checked parameters

		wchar_t* const trg = target.GetStart();

		for (size_t i = 0; i < off; i++)
			trg[i] = Space;

		const size_t written = src.copy(trg + off, maxCount);

		for (size_t i = off + written; i < target.Length; i++)
			trg[i] = Space;

		return written;
	}


	static size_t FillTarget(const std::wstring_view& src, size_t off, StringSection target)
	{
		return FillTarget(src, off, src.length(), target);
	}


	// Checked helper: must have enough space - preferred padding still can be reduced down to 0
	static void AlignText(const std::wstring_view& src, const PaddedAlignment& aln, StringSection target)
	{
		LOGIC_ASSERT (PlaceText(src, target, aln));
	}


	bool PlaceText(std::wstring_view src, StringSection target, PaddedAlignment aln)
	{
		if (target.Length < src.length())
			return false;

		auto pos = aln.CalcPosition(src.length(), target.Length, false);
		DBG_ASSERT (pos.textStart + src.length() <= target.Length);

		size_t copied = FillTarget(src, pos.textStart, target);
		DBG_ASSERT (copied == src.length());

		return true;
	}


	// padding takes precedence here
	size_t PlaceTruncableText(std::wstring_view src, StringSection target, PaddedAlignment aln)
	{
		DBG_ASSERT_M (aln.pad < target.Length, "Unintended padding parameter?");		// should run fine though

		auto pos = aln.CalcPosition(src.length(), target.Length);
		
		return FillTarget(src, pos.textStart, pos.textLen, target);
	}


	std::wstring AlignCenter(size_t lineLen, std::wstring_view src)
	{
		LOGIC_ASSERT (src.length() <= lineLen);

		std::wstring s (lineLen, ' ');

		const size_t pad = (lineLen - src.length()) / 2;
		src.copy(s.data() + pad, src.length());

		return s;
	}

#pragma endregion



#pragma region PlaceNumber

	static bool AlignIntegerChars(const wchar_t* digits, int printed, StringSection target, const PaddedAlignment& aln, const std::wstring_view& overrunSymb)
	{
		LOGIC_ASSERT (printed > 0);

		if (printed <= target.Length)
		{
			AlignText({ digits, Cast::Implied<unsigned>(printed) }, aln, target);
			return true;
		}
		PlaceTruncableText(overrunSymb, target, aln);
		return false;
	}


	bool PlaceNumber(uint32_t value, StringSection target, PaddedAlignment aln, std::wstring_view overrunSymb)
	{
		wchar_t num[Int32MaxChars + 1];
		int		len = PrintTo(num, L"%u", value);

		return AlignIntegerChars(num, len, target, aln, overrunSymb);
	}


	bool PlaceNumber(int32_t value, SignUsage sign, StringSection target, PaddedAlignment aln, std::wstring_view overrunSymb)
	{
		if (NonDefault(sign & SignUsage::ForbidNegativeValues) && value < 0)
		{
			PlaceTruncableText(overrunSymb, target, aln);
			return false;
		};

		const wchar_t* format = NonDefault(sign & SignUsage::PrependPlus)
			? L"%+d" 
			: L"%d";

		wchar_t num[Int32MaxChars + 1];
		int		len = PrintTo(num, format, value);

		return AlignIntegerChars(num, len, target, aln, overrunSymb);
	}


	bool PlaceNumber(int32_t value, StringSection target, PaddedAlignment aln, std::wstring_view overrunSymb)
	{
		return PlaceNumber(value, SignUsage {}, target, aln, overrunSymb);
	}




	static bool TryAlignDecimal(const std::wstring_view& num, StringSection target, const PaddedAlignment& aln, bool preferDecimals)
	{
		LOGIC_ASSERT (aln.pad < target.Length);
		DBG_ASSERT	 (0 < num.length());

		const size_t pointPos  = num.find(DecimalSeparator);
		const bool	 hasPoint  = pointPos != num.npos;
		const size_t wholePart = hasPoint ? pointPos : num.length();
		if (target.Length < wholePart)
			return false;

		const bool letFraction = hasPoint 
			&& target.Length > wholePart + (preferDecimals ? 2 : 1 + aln.pad);

		const size_t limit  = target.Length - (!preferDecimals * aln.pad);
		const size_t cpyLen = letFraction
			? std::min(num.length(), limit)		// cut fraction to maintain preferred padding / target length
			: wholePart;						// must fit, that we established

		DBG_ASSERT (cpyLen <= target.Length);

		AlignText(num.substr(0, cpyLen), aln, target);
		return true;
	}


	bool PlaceNumber(double value, DecimalUsage opts, StringSection target, PaddedAlignment aln, std::wstring_view overrunSymb)
	{
		DBG_ASSERT(opts.maxDecimals <= MaxFractionDigits);

		if (NonDefault(opts.sign & SignUsage::ForbidNegativeValues) && std::signbit(value))
		{
			PlaceTruncableText(overrunSymb, target, aln);
			return false;
		};

		// Disable rounding, we want to consistently truncate here. Eg. Mach 0.95 != Mach 1.0
		// MAYBE: should be controllable via DecimalUsage - doing so would require manual rounding after truncation to stay consistent.
		struct RoundModeGuard {
			const int origMode = std::fegetround();
			RoundModeGuard()   { std::fesetround(FE_TOWARDZERO); }
			~RoundModeGuard()  { std::fesetround(origMode);	   }
		} modeGuard;

		const wchar_t* format = NonDefault(opts.sign & SignUsage::PrependPlus) 
			? L"%+.*f" 
			: L"%.*f";

		wchar_t num[PlaceDoubleMaxChars + 1];
		int len = PrintTo(num, format, opts.maxDecimals, value);
		if (len > 0)
		{
			std::wstring_view unrounded { num, Cast::Implied<unsigned>(len) };
			if (TryAlignDecimal(unrounded, target, aln, opts.preferDecimalsOverPadding))
				return true;
		}
		PlaceTruncableText(overrunSymb, target, aln);
		return false;
	}

#pragma endregion


}	//namespace Utils::String
