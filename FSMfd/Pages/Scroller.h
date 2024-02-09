#pragma once

#include "DirectOutputHelper/X52Page.h"
#include "Utils/CastUtils.h"



namespace FSMfd::Pages
{

	/// Helper to display 3< rows on X52 display by allowing to scroll them.
	class Scroller {
		DOHelper::X52Output::Page&	target;
		std::vector<std::wstring>	unseenLines;
		unsigned					lineCount;
		unsigned					displayPos;
		bool						useEmptyGuards;

	public:
		/// @param useEmptyGuardLines:  allow scroll over edges by 1 line to indicate end of page
		/// @param initialSize:         virtual lines to start with (min. 1, can increase later)
		Scroller(DOHelper::X52Output::Page& target, bool useEmptyGuardLines = true, unsigned initialSize = 1);
		Scroller(DOHelper::X52Output::Page& target, unsigned initialSize);

		bool	 HasEmptyGuardLines() const  { return useEmptyGuards; }
		unsigned LineCount()		  const	 { return lineCount; }
		unsigned FirstDisplayedLine() const;
		unsigned LastDisplayedLine()  const;

		bool				IsDisplayed	(unsigned i) const;
		const std::wstring& GetLine		(unsigned i) const;
		std::wstring&		ModLine		(unsigned i);
		void 				SetLine		(unsigned i, std::wstring text);
		
		bool				IsAtop()	const;
		bool				IsBottom()	const;

		bool ScrollUp();
		bool ScrollDown();

		void AddLines(unsigned count);
		void EnsureLineCount(unsigned totalCount);
	};


}	// namespace FSMfd::Pages
