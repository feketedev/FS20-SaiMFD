#include "StringUtils.h"

#include "BasicUtils.h"
#include "Debug.h"

#include <cfenv>
#include <locale>
#include <utility>


namespace Utils::String
{

#pragma region Consts+Locale

	constexpr wchar_t  Space = L' ';	 // clear character for alignments

	constexpr unsigned Int32MaxChars	 = std::numeric_limits<int32_t>::digits10 + 1;		// mind the '-'

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
		pos		{ pos },
		length	{ length }
	{
		LOGIC_ASSERT (pos + length <= buffer.length());		// wstring excludes \0
	}


	StringSection::StringSection(std::wstring& buffer, size_t pos) :
		StringSection { buffer, pos, buffer.length() - pos }
	{
	}


	StringSection StringSection::FollowedBy(size_t nextLength) const
	{
		return { buffer, pos + length, nextLength };
	}


	StringSection StringSection::SubSection(size_t offset, size_t length) const
	{
		LOGIC_ASSERT (offset + length <= this->length);

		return { buffer, pos + offset, length };
	}


	wchar_t* StringSection::GetStart() const
	{
		return buffer.data() + pos;
	}


	wchar_t * StringSection::GetLast() const
	{
		LOGIC_ASSERT (length > 0);
		return GetStart() + length - 1;
	}


	std::wstring_view StringSection::AsStringView() const
	{
		return { GetStart(), length };
	}


	void StringSection::FillIn(std::wstring_view src)
	{
		DBG_ASSERT	   (src.length() == length);
		LOGIC_ASSERT_M (buffer.size() >= pos + length, "Buffer shortened in meantime?")

		wchar_t* trg = GetStart();
		for (size_t i = 0; i < src.length(); i++)
			trg[i] = src[i];
	}


	void StringSection::FillWith(wchar_t c)
	{
		wchar_t* trg = GetStart();
		for (size_t i = 0; i < length; i++)
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
		DBG_ASSERT (off + maxCount <= target.length);	// expects checked parameters

		wchar_t* const trg = target.GetStart();

		for (size_t i = 0; i < off; i++)
			trg[i] = Space;

		const size_t written = src.copy(trg + off, maxCount);

		for (size_t i = off + written; i < target.length; i++)
			trg[i] = Space;

		return written;
	}


	static size_t FillTarget(const std::wstring_view& src, size_t off, StringSection target)
	{
		return FillTarget(src, off, src.length(), target);
	}


	// Checked helper: must have enough space - preferred padding still can be reduced down to 0
	static void AlignText(const std::wstring_view& src, const PaddedAlignment& aln, const StringSection& target)
	{
		LOGIC_ASSERT (PlaceText(src, target, aln));
	}


	bool PlaceText(const std::wstring_view& src, StringSection target, const PaddedAlignment& aln)
	{
		if (target.length < src.length())
			return false;

		auto pos = aln.CalcPosition(src.length(), target.length, false);
		DBG_ASSERT (pos.textStart + src.length() <= target.length);

		size_t copied = FillTarget(src, pos.textStart, target);
		DBG_ASSERT (copied == src.length());

		return true;
	}


	// padding takes precedence here
	size_t PlaceTruncableText(const std::wstring_view& src, StringSection target, const PaddedAlignment& aln)
	{
		DBG_ASSERT_M (aln.pad < target.length, "Unintended padding parameter?");		// should run fine though

		auto pos = aln.CalcPosition(src.length(), target.length);
		
		return FillTarget(src, pos.textStart, pos.textLen, target);
	}


	std::wstring AlignCenter(size_t lineLen, const std::wstring_view& src)
	{
		LOGIC_ASSERT (src.length() <= lineLen);

		std::wstring s (lineLen, ' ');

		const size_t pad = (lineLen - src.length()) / 2;
		src.copy(s.data() + pad, src.length());

		return s;
	}

#pragma endregion



#pragma region PlaceNumber

	bool PlaceNumber(int32_t value, StringSection target, const PaddedAlignment& aln, const std::wstring_view& overrunSymb)
	{
		wchar_t num[Int32MaxChars + 1];
		int		len = PrintTo(num, L"%d", value);
		LOGIC_ASSERT (0 < len);

		if (len <= target.length)
		{
			AlignText({ num, static_cast<size_t> (len) }, aln, target);
			return true;
		}
		PlaceTruncableText(overrunSymb, target, aln);
		return false;
	}



	static bool TryAlignDecimal(const std::wstring_view& num, StringSection target, const PaddedAlignment& aln, bool preferDecimals)
	{
		LOGIC_ASSERT (aln.pad < target.length);
		DBG_ASSERT	 (0 < num.length());

		const size_t pointPos  = num.find(DecimalSeparator);
		const bool	 hasPoint  = pointPos != num.npos;
		const size_t wholePart = hasPoint ? pointPos : num.length();
		if (target.length < wholePart)
			return false;

		const bool letFraction = hasPoint 
			&& target.length > wholePart + (preferDecimals ? 2 : 1 + aln.pad);

		const size_t limit  = target.length - (!preferDecimals * aln.pad);
		const size_t cpyLen = letFraction
			? std::min(num.length(), limit)		// cut fraction to maintain preferred padding / target length
			: wholePart;						// must fit, that we established

		DBG_ASSERT (cpyLen <= target.length);

		AlignText(num.substr(0, cpyLen), aln, target);
		return true;
	}


	bool PlaceNumber(double value, DecimalLimit limit, StringSection target, const PaddedAlignment& aln, const std::wstring_view& overrunSymb)
	{
		DBG_ASSERT(limit.maxDecimals <= MaxFractionDigits);

		// Disable rounding, we want to consistently truncate here. Eg. Mach 0.95 != Mach 1.0
		struct RoundModeGuard {
			const int origMode = std::fegetround();
			RoundModeGuard()   { std::fesetround(FE_TOWARDZERO); }
			~RoundModeGuard()  { std::fesetround(origMode);	   }
		} modeGuard;


		wchar_t num[PlaceDoubleMaxChars + 1];
		int len = PrintTo(num, L"%.*f", limit.maxDecimals, value);
		if (len > 0)
		{
			std::wstring_view unrounded { num, static_cast<size_t>(len) };
			if (TryAlignDecimal(unrounded, target, aln, limit.preferDecimalsOverPadding))
				return true;
		}
		PlaceTruncableText(overrunSymb, target, aln);
		return false;
	}

#pragma endregion


}	//namespace Utils::String
