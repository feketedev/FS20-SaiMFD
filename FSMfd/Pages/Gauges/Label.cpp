#include "Label.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{

	Label::Label(std::wstring text) :
		StackableGauge { Practically<unsigned>(text.length()), 1, {} },
		text		   { std::move(text) }
	{
	}


	void Label::Clean(DisplayArea& display)
	{
		DBG_ASSERT (display.size == 1);

		display[0].FillIn(text);
	}


	void Label::Update(const SimvarSublist& , DisplayArea&)
	{
	}


}	// namespace FSMfd::Pages
