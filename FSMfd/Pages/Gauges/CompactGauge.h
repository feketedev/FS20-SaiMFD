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
		const std::wstring		label;
		const std::wstring		unitText;
		const unsigned			unitPadding;

		std::array<Utils::String::StringSection, 3>		DissectLine(Utils::String::StringSection& display) const;
		std::wstring_view								GetUnit() const;

	public:
		/// Create a full-line gauge.
		CompactGauge(const DisplayVar&);

		/// Create a limited length gauge.
		CompactGauge(unsigned displayLength, const DisplayVar&);


		void Clean(DisplayArea&)						const override;
		void Update(const SimvarSublist&, DisplayArea&)	const override;

	};

}	// namespace FSMfd::Pages
