#pragma once

	/*  Part of FS20-SaiMFD			   *
	 *  Copyright 2023 Norbert Fekete  *
     *                                 *
	 *  Released under GPLv3.		   */


#include <string>


namespace Utils::String
{

	enum class Align : uint8_t { Left, Right };
	

	/// Request to align text in a direction but keep a given padding next to it,
	/// as far as space allows.
	/// @remarks
	///	  This allows to "break the margin" as last resort 
	///   to squeeze some extra digits on a small screen.
	struct PaddedAlignment {
		Align		direction;
		size_t		pad;

		constexpr PaddedAlignment(Align dir, size_t pad = 0) : direction { dir }, pad { pad }
		{
		}


		struct Position {
			size_t textStart, textLen, padRemain;
		};

		/// Position a desired text (which might undergo truncation)
		/// into a field of given length, satisfying this alignment.
		/// @param paddingHasPrecedence: truncate copied text first, only than padding space.
		Position CalcPosition(size_t textLen, size_t fieldLen, bool paddingHasPrecedence = true) const;
	};



	/// Flags that control printing of signed numbers.
	enum class SignUsage : uint8_t {
		Default				 = 0,
		PrependPlus			 = 1,
		ForbidNegativeValues = 2,	// Simple solution to acute problems
		//AvoidNegativeZero	 = 4	// MAYBE: if needed, more complicated
	};


	/// Controls truncation of decimal numbers.
	struct DecimalUsage {
		SignUsage	sign;

		/// Requested precision, truncated further as needed. Max numeric_limits{double}::digits10.
		uint8_t		maxDecimals;

		/// When room is scarce, violate padding before cutting decimals.
		bool		preferDecimalsOverPadding;


		constexpr DecimalUsage(uint8_t max, bool preferDecimals = false) :
			DecimalUsage { max, SignUsage::Default, preferDecimals }
		{
		}

		constexpr DecimalUsage(uint8_t max, SignUsage su, bool preferDecimals = false) :
			sign { su }, maxDecimals { max }, preferDecimalsOverPadding { preferDecimals }
		{
		}
	};



	/// Modifiable part of a wstring buffer. Usually target of string operations.
	struct StringSection {
		std::wstring&	buffer;
		const size_t	Pos;
		const size_t	Length;

		StringSection(std::wstring& buffer, size_t pos, size_t length);
		StringSection(std::wstring& buffer, size_t pos = 0);			// pos -> buffer end

		StringSection		FollowedBy(size_t length) const;
		StringSection		SubSection(size_t offset, size_t length) const;

		wchar_t&			operator[](size_t) const;
		wchar_t*			GetStart()		   const;
		wchar_t*			GetLast()		   const;
		std::wstring_view	AsStringView()	   const;

		void FillIn(std::wstring_view src);
		void FillWith(wchar_t);
	};



	// ------------------------------------------------------------------------------------------------------

	/// Character used by these utils during the execution of the program. (Read once from locale.)
	extern const wchar_t DecimalSeparator;


	/// Reasonable, but arbitrary absolute limits of PlaceNumber(double). From them false might be returned.
	extern const double	PlaceDoubleUpperLimit;
	extern const double	PlaceDoubleLowerLimit;


	/// Put space-padded digits to a field specified as a section of a string buffer.
	/// @param target 		Section of buffer to be updated.
	/// @param overrunSymb 	Put this string (trimmed to target length) to signal if @p target was too short for the required digits.
	/// @returns 			Had enough space for the digits.
	bool PlaceNumber(uint32_t value, StringSection target, PaddedAlignment = Align::Right, std::wstring_view overrunSymb = L"##");
	bool PlaceNumber(int32_t  value, StringSection target, PaddedAlignment = Align::Right, std::wstring_view overrunSymb = L"##");
	bool PlaceNumber(int32_t  value, SignUsage, StringSection target, PaddedAlignment = Align::Right, std::wstring_view overrunSymb = L"##");
	
	/// Put space-padded digits (in decimal form) to a field specified as a section of a string buffer.
	/// @param target 		Section of buffer to be updated.
	/// @param overrunSymb 	Put this string (trimmed to target length) to signal if @p target was too short for the whole digits.
	/// @returns 			Had enough space at least for the digits preceeding the decimal point.
	bool PlaceNumber(double value, DecimalUsage, StringSection target, PaddedAlignment = Align::Right, std::wstring_view overrunSymb = L"##");
	

	/// Put space-padded text to a field specified as a section of a string buffer.
	/// @returns 	Number of characters copied from @p src.
	size_t	PlaceTruncableText(std::wstring_view src, StringSection target, PaddedAlignment);

	/// Put space-padded, non-truncable text to a field specified as a section of a string buffer.
	/// @returns 	Did copy @src, as @p target had enough room for it.
	bool	PlaceText(std::wstring_view src, StringSection target, PaddedAlignment);



	std::wstring AlignCenter(size_t lineLen, std::wstring_view text);



	// ------------------------------------------------------------------------------------------------------

	// X52 only supports old ASCII characters, nothing fancy needed...
	inline std::wstring AsDumbWString(const std::string_view& str)
	{
		return { str.begin(), str.end() };
	}


	inline SignUsage operator|(SignUsage a, SignUsage b)	{ return static_cast<SignUsage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
	inline SignUsage operator&(SignUsage a, SignUsage b)	{ return static_cast<SignUsage>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
	inline bool		 NonDefault(SignUsage u)				{ return u != SignUsage::Default; }


	inline wchar_t*			begin(StringSection& s)			{ return s.GetStart(); }
	inline wchar_t*			end(StringSection& s)			{ return s.GetStart() + s.Length; }
	inline const wchar_t*	begin(const StringSection& s)	{ return s.GetStart(); }
	inline const wchar_t*	end(const StringSection& s)		{ return s.GetStart() + s.Length; }


}	// namespace Utils::String
