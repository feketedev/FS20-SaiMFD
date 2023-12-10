#pragma once

#include "StackableGauge.h"
#include "Pages/SimvarPrinter.h"
#include <array>



namespace Utils::String { struct StringSection; }

namespace FSMfd::Pages
{

	/// Simplistic gauge with optional label and unit around the measured value.
	class CompactGauge : public StackableGauge
	{
		const SimvarPrinter		printValue;
		const unsigned			unitPadding;
		const std::wstring		label;
		const std::wstring		unitSymbol;

		std::array<Utils::String::StringSection, 3>		DissectLine(Utils::String::StringSection& display) const;

	public:
		/// Create a full-line gauge with label and indicated unit.
		/// @remark Long numbers can extend into the leading spaces of the unitText.
		CompactGauge(const DisplayVar&);

		/// Create a limited length gauge with label and indicated unit.
		/// @remark Long numbers can extend into the leading spaces of the unitText.
		CompactGauge(unsigned displayLength, const DisplayVar&);

		/// Create a limited length gauge (value only).
		CompactGauge(unsigned displayLength, const SimVarDef&);


		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;
	};

}	// namespace FSMfd::Pages
