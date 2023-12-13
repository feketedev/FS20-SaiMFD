#include "CompactGauge.h"

#include "SimVarDef.h"
#include "DirectOutputHelper/X52Page.h"		// DisplayLen
#include "Pages/SimvarPrinter.h"
#include "Utils/StringUtils.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace Utils::String;

	constexpr uint8_t SignificantDigits = std::numeric_limits<double>::digits10;


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
		unitPadding    { CountLeadingSpaces(dv.unitText) },
		label          { dv.text },
		unitSymbol     { dv.unitText.substr(unitPadding) }
	{
		LOGIC_ASSERT_M (label.length() + dv.unitText.length() - unitPadding < displayLength,
						"Insufficient space for defined texts.");
	}


	CompactGauge::CompactGauge(unsigned displayLength, const SimVarDef& simvar) :
		CompactGauge { displayLength, DisplayVar { L"", simvar, L"", std::min<uint8_t>(displayLength - 2, SignificantDigits) }}
	{
	}


	std::array<StringSection, 3>	CompactGauge::DissectLine(StringSection& display) const
	{
		const size_t valueRoom = DisplayWidth - label.length() - unitSymbol.length();

		StringSection labelTrg = display.SubSection(0, label.length());
		StringSection valueTrg = labelTrg.FollowedBy(valueRoom);
		StringSection unitTrg  = valueTrg.FollowedBy(unitSymbol.length());

		return { labelTrg, valueTrg, unitTrg };
	}


	void CompactGauge::Clean(DisplayArea& display)
	{
		DBG_ASSERT (display.size == 1);

		auto [labelTrg, valueTrg, unitTrg] = DissectLine(display[0]);

		labelTrg.FillIn(label);
		valueTrg.FillWith(L' ');
		unitTrg.FillIn(unitSymbol);
	}


	void CompactGauge::Update(const SimvarSublist& measurement, DisplayArea& display)
	{
		DBG_ASSERT (measurement.VarCount == 1u);
		DBG_ASSERT (display.size == 1);

		auto [_, valueTrg, __] = DissectLine(display[0]);

		printValue(measurement[0], valueTrg, { Align::Right, unitPadding });
	}

}	// namespace FSMfd::Pages
