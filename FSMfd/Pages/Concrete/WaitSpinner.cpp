#include "WaitSpinner.h"

#include "Utils/StringUtils.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace Utils::String;

	constexpr unsigned Padding = 1;


	WaitSpinner::WaitSpinner(std::wstring title, uint32_t id, std::wstring bar) :
		Page      { id },
		Title     { std::move(title) },
		Bar		  { std::move(bar) },
		stepCount { DisplayLength - 2 * Padding }
	{
		LOGIC_ASSERT (Bar.length() < DisplayLength - 2 * Padding);

		SetLine(0, AlignCenter(DisplayLength, Title));
		SetLine(1, std::wstring(DisplayLength, ' '));
	}


	void WaitSpinner::SetStatus(std::wstring s)
	{
		if (status == s)
			return;

		status = std::move(s);
		
		if (status.length() < DisplayLength)
			SetLine(2, AlignCenter(DisplayLength, status));
		else
			SetLine(2, status);
	}


	void WaitSpinner::DrawAnimation()
	{
		step = (step + 1) % stepCount;

		unsigned	   pos	 = step + Padding;
		const unsigned erase = (Padding < pos)
							  ? pos - 1
							  : DisplayLength - Padding - 1;

		std::wstring& line = ModLine(1);
		line[erase] = L' ';
		for (wchar_t c : Bar)
		{
			line[pos++] = c;
			if (pos >= DisplayLength - Padding)
				pos = Padding;
		}

		if (IsActive())
			DrawLines();
	}


}	//namespace FSMfd::Pages
