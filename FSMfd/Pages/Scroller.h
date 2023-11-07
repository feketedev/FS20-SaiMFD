#pragma once

#include "DirectOutputHelper/X52Page.h"



namespace FSMfd::Pages {


	/// Helper to display 3< rows on X52 display by allowing to scroll them.
	class Scroller {
		DOHelper::X52Output::Page&	target;
		std::vector<std::wstring>	unseenLines;
		unsigned					displayPos;
		bool						emptyGuardLines;


	public:
		Scroller(DOHelper::X52Output::Page& target, unsigned size, bool useEmptyGuardLines = true);

		bool	 HasEmptyGuardLines() const  { return emptyGuardLines; }
		unsigned LineCount()		  const;
		unsigned FirstDisplayedLine() const;
		unsigned LastDisplayedLine()  const;

		bool				IsDisplayed	(unsigned i) const;
		const std::wstring& GetLine		(unsigned i) const;
		std::wstring&		ModLine		(unsigned i);
		void 				SetLine		(unsigned i, std::wstring text);

		bool ScrollUp();
		bool ScrollDown();
	};




	inline unsigned Scroller::LineCount() const
	{
		return unseenLines.size() + (emptyGuardLines ? 1 : 3);
	}


}	// namespace FSMfd::Pages
