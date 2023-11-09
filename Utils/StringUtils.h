#pragma once

	/*  Part of FS20-SaiMFD			   *
	 *  Copyright 2023 Norbert Fekete  *
     *                                 *
	 *  Released under GPLv3.		   */


#include <string>


namespace Utils::String {

	enum class Align { Left, Right };
	

	/// Request to align text in a direction but keep a given padding next to it,
	/// as far as space allows.
	/// @remarks
	///	  This allows to "break the margin" as last resort 
	///   to squeeze some extra digits on a small screen.
	struct PaddedAlignment {
		const Align		direction;
		const size_t	pad;

		PaddedAlignment(Align dir, size_t pad = 0) : direction { dir }, pad { pad }
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



	/// Modifiable part of a wstring buffer. Usually target of string operations.
	struct StringSection {
		std::wstring&	buffer;
		const size_t	pos;
		const size_t	length;

		StringSection(std::wstring& buffer, size_t pos, size_t length);
		StringSection(std::wstring& buffer, size_t pos = 0);			// pos -> buffer end

		StringSection		FollowedBy(size_t length) const;

		wchar_t*			GetStart()		const;
		std::wstring_view	AsStringView()	const;

		void FillWith(std::wstring_view src);
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
	bool PlaceNumber(int32_t value, StringSection target, const PaddedAlignment& = Align::Right, const std::wstring_view& overrunSymb = L"##");
	
	/// Put space-padded digits (in decimal form) to a field specified as a section of a string buffer.
	/// @param maxDecimals  Requested precision, truncated as needed. Max numeric_limits<double>::digits10.
	/// @param target 		Section of buffer to be updated.
	/// @param overrunSymb 	Put this string (trimmed to target length) to signal if @p target was too short for the whole digits.
	/// @returns 			Had enough space at least for the digits preceeding the decimal point.
	bool PlaceNumber(double value, unsigned maxDecimals, StringSection target, const PaddedAlignment& = Align::Right, const std::wstring_view& overrunSymb = L"##");

	

	/// Put space-padded text to a field specified as a section of a string buffer.
	/// @returns 	Number of characters copied from @p src.
	size_t	PlaceTruncableText(const std::wstring_view& src, StringSection target, const PaddedAlignment&);

	/// Put space-padded, non-truncable text to a field specified as a section of a string buffer.
	/// @returns 	Did copy @src, as @p target had enough room for it.
	bool	PlaceText(const std::wstring_view& src, StringSection target, const PaddedAlignment&);




	template <size_t LineLen, size_t TLen>
	std::wstring AlignCenter(const wchar_t (&textLiter)[TLen])
	{
		static_assert(TLen <= LineLen + 1);

		std::wstring s (LineLen, ' ');

		constexpr size_t len = TLen - 1;
		constexpr size_t pad = (LineLen - len) / 2;

		for (size_t i = 0; i < len; i++)
			s[pad + i] = textLiter[i];

		return s;
	};


	// TODO: Kell?
	std::wstring AlignCenter(size_t lineLen, const std::wstring_view& text);



}	// namespace Utils::String