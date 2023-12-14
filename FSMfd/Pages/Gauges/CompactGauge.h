#pragma once

#include "StackableGauge.h"
#include "Pages/SimvarPrinter.h"
#include <array>



namespace Utils::String { struct StringSection; }

namespace FSMfd::Pages
{

	/// Simplistic gauge with optional label and unit around the measured value.
	class CompactGauge : public StackableGauge {

		const SimvarPrinter		printValue;
		const std::wstring		label;
		const std::wstring		unitSymbol;
		const bool				stickyUnit;

		std::array<Utils::String::StringSection, 3>		DissectLine(Utils::String::StringSection& display) const;

	public:
		/// Create a full-line gauge with label and indicated unit.
		/// @param stickyUnit	false: unit is right justified, 1 padding / true: unit closely follows value / default by unit
		CompactGauge(const DisplayVar&, optional<bool> stickyUnit = Nothing);

		/// Create a limited length gauge with label and indicated unit.
		/// @param stickyUnit	false: unit is right justified, 1 padding / true: unit closely follows value / default by unit
		CompactGauge(unsigned displayLength, const DisplayVar&, optional<bool> stickyUnit = Nothing);

		/// Create a limited length gauge (value only).
		CompactGauge(unsigned displayLength, const SimVarDef&);


		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;
	};

}	// namespace FSMfd::Pages
