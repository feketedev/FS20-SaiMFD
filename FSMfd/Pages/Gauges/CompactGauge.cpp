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


	static bool IsStickyUnit(const std::wstring& unitSymbol)
	{
		if (unitSymbol.length() > 1)
			return false;

		return unitSymbol.empty()
			|| unitSymbol[0] == L'°'
			|| unitSymbol[0] == L'%';
	}


	CompactGauge::CompactGauge(const DisplayVar& dv, optional<bool> stickyUnit) :
		CompactGauge { DOHelper::X52Output::Page::DisplayLength, dv, stickyUnit }
	{
	}


	CompactGauge::CompactGauge(unsigned length, const DisplayVar& dv, optional<bool> stickyUnit) :
		StackableGauge { length, 1, { dv.definition } },
		printValue     { CreateValuePrinterFor(dv) },
		stickyUnit     { stickyUnit.value_or(IsStickyUnit(dv.unitText)) },
		label          { dv.text },
		unitSymbol	   { dv.unitText }
	{
		LOGIC_ASSERT_M (label.length() + unitSymbol.length() < length,
						"Insufficient space for defined texts."		 );
	}


	CompactGauge::CompactGauge(unsigned length, const SimVarDef& simvar) :
		CompactGauge { length, DisplayVar { L"", simvar, std::min<uint8_t>(length - 2, SignificantDigits), L"" }}
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

		auto [_, valueTrg, unitLast] = DissectLine(display[0]);

		// TODO: support left alignment? (DisplayVar?)
		unsigned padding = stickyUnit ? 0 : 1;
		printValue(measurement[0], valueTrg, { Align::Right, padding });
	}

}	// namespace FSMfd::Pages
