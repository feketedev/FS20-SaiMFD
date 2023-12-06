#pragma once

#include "DirectOutputHelper/X52Page.h"



namespace FSMfd::Pages {


	/// Helper to display 3< rows on X52 display by allowing to scroll them.
	class Scroller {
		DOHelper::X52Output::Page&	target;
		std::vector<std::wstring>	unseenLines;
		unsigned					displayPos;
		bool						useEmptyGuards;

	public:
		Scroller(DOHelper::X52Output::Page& target, unsigned size = 3, bool useEmptyGuardLines = true);

		bool	 HasEmptyGuardLines() const  { return useEmptyGuards; }
		unsigned LineCount()		  const;
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




	inline unsigned Scroller::LineCount() const
	{
		return static_cast<unsigned>(unseenLines.size())
			 + (useEmptyGuards ? 1u : 3u);
	}


}	// namespace FSMfd::Pages
