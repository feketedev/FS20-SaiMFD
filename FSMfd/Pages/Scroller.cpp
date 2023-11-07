#include "Scroller.h"
#include "FSMfdTypes.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages {


Scroller::Scroller(DOHelper::X52Output::Page& target, unsigned size, bool useEmptyGuardLines) :
	target			{ target },
	emptyGuardLines { useEmptyGuardLines },
	displayPos		{ useEmptyGuardLines ? 1u : 0u }
{
	LOGIC_ASSERT_M (size >= 3, "Narrowing the screen is not supported.");
	unseenLines.resize(size - 3 + 2 * (int)emptyGuardLines);
}


// Public index of currently displayed line on middle of the screen.
static unsigned PubMidIdx(unsigned displayPos, bool emptyGuardLines)
{
	return displayPos + 1 - (int)emptyGuardLines;
}


unsigned Scroller::FirstDisplayedLine() const
{
	unsigned mid = PubMidIdx(displayPos, emptyGuardLines);
	return std::max(mid, 1u) - 1;
}


unsigned Scroller::LastDisplayedLine() const
{
	unsigned mid = PubMidIdx(displayPos, emptyGuardLines);
	return std::min(mid + 1, LineCount() - 1);
}


bool Scroller::IsDisplayed(unsigned line) const
{
	return displayPos <= line + (int)emptyGuardLines
		&& displayPos + 3 > line + (int)emptyGuardLines;
}


/// @returns [Index, DisplayedNotUnseen]
static pair<unsigned, bool>	GetBuffIndex(unsigned i, unsigned displayPos, bool emptyGuardLines)
{
	const unsigned offset = (unsigned)emptyGuardLines;
	const unsigned mid	  = PubMidIdx(displayPos, emptyGuardLines);

	if (i + 1 < mid)
		return { i + offset, false };

	if (mid + 1 < i)
		return { i + offset - 3, false };

	return { i + 1 - mid, true };
}


const std::wstring& 	Scroller::GetLine(unsigned i) const
{
	LOGIC_ASSERT (i < LineCount());

	auto [b, displayed] = GetBuffIndex(i, displayPos, emptyGuardLines);

	return displayed 
		? target.GetLine(b) 
		: unseenLines[b];
}


std::wstring&			Scroller::ModLine(unsigned i)
{
	LOGIC_ASSERT (i < LineCount());

	auto [b, displayed] = GetBuffIndex(i, displayPos, emptyGuardLines);

	return displayed
		? target.ModLine(b)
		: unseenLines[b];
}


void Pages::Scroller::SetLine(unsigned i, std::wstring text)
{
	LOGIC_ASSERT (i < LineCount());

	auto [b, displayed] = GetBuffIndex(i, displayPos, emptyGuardLines);
	if (displayed)
		target.SetLine(b, std::move(text), true);
	else
		unseenLines[b] = std::move(text);
}


bool Scroller::ScrollUp()
{
	if (displayPos < 1)
		return false;

	std::wstring& l0 = target.ModLine(0);
	std::wstring& l1 = target.ModLine(1);
	std::wstring& l2 = target.ModLine(2);
	l2.swap(l1);
	l1.swap(l0);
	l0.swap(unseenLines[--displayPos]);
	return true;
}


bool Scroller::ScrollDown()
{
	if (displayPos >= unseenLines.size())
		return false;

	std::wstring& l0 = target.ModLine(0);
	std::wstring& l1 = target.ModLine(1);
	std::wstring& l2 = target.ModLine(2);
	l0.swap(l1);
	l1.swap(l2);
	l2.swap(unseenLines[displayPos++]);
	return true;
}


}	// namespace FSMfd::Pages