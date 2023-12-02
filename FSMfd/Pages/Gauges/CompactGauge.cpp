#include "CompactGauge.h"

#include "SimVarDef.h"
#include "DirectOutputHelper/X52Page.h"		// DisplayLen
#include "Pages/SimvarPrinter.h"
#include "Utils/StringUtils.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace Utils::String;


	static unsigned CountLeadingSpaces(const std::wstring& s)
	{
		unsigned i = 0;
		while (i < s.length() && s[i] == L' ')
			++i;

		return i;
	}


	CompactGauge::CompactGauge(const DisplayVar& dv) :
		CompactGauge { DOHelper::X52Output::Page::DisplayLength, dv }
	{
	}


	CompactGauge::CompactGauge(unsigned displayLength, const DisplayVar& dv) :
		StackableGauge { displayLength, 1, { dv.definition } },
		printValue     { CreateValuePrinterFor(dv) },
		label          { dv.text },
		unitText       { dv.unitText },
		unitPadding    { CountLeadingSpaces(unitText) }
	{
		LOGIC_ASSERT_M (label.length() + unitText.length() - unitPadding < displayLength,
						"Insufficient space for defined texts.");
	}


	std::array<StringSection, 3>	CompactGauge::DissectLine(StringSection& display) const
	{
		const unsigned valueRoom = DisplayWidth - label.length() - GetUnit().length();

		StringSection labelTrg = display.SubSection(0, label.length());
		StringSection valueTrg = labelTrg.FollowedBy(valueRoom);
		StringSection unitTrg  = valueTrg.FollowedBy(GetUnit().length());

		return { labelTrg, valueTrg, unitTrg };
	}


	std::wstring_view CompactGauge::GetUnit() const
	{
		return { unitText.data() + unitPadding, unitText.length() - unitPadding };
	}


	void CompactGauge::Clean(DisplayArea& display) const
	{
		DBG_ASSERT (display.size == 1);

		auto [labelTrg, valueTrg, unitTrg] = DissectLine(display[0]);

		labelTrg.FillIn(label);
		valueTrg.FillWith(L' ');
		unitTrg.FillIn(unitText);
	}


	void CompactGauge::Update(const SimvarSublist& measurement, DisplayArea& display) const
	{
		DBG_ASSERT (measurement.VarCount == 1u);
		DBG_ASSERT (display.size == 1);

		auto [_, valueTrg, __] = DissectLine(display[0]);

		printValue(measurement[0], valueTrg, { Align::Right, unitPadding });
	}

}	// namespace FSMfd::Pages
