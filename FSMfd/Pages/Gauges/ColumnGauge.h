#pragma once

#include "StackableGauge.h"
#include "Pages/SimvarPrinter.h"
#include "Utils/StringUtils.h"



namespace FSMfd::Pages
{

	/// A column with optional label on top, followed by 1 value per row underneeth.
	class ColumnGauge final : public StackableGauge {

		const std::wstring					header;
		const std::vector<SimvarPrinter>	printers;


		bool HasHeader() const				{ return !header.empty(); }

	public:
		/// Create a column in the width of @p header.
		ColumnGauge(std::wstring header, std::vector<SimVarDef>, Utils::String::DecimalUsage = 2);
		ColumnGauge(std::wstring header, std::vector<SimVarDef>, Utils::String::SignUsage);

		/// Create a fixed width column with no header.
		ColumnGauge(unsigned width, std::vector<SimVarDef>, Utils::String::DecimalUsage = 2);
		ColumnGauge(unsigned width, std::vector<SimVarDef>, Utils::String::SignUsage);


		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;
	};


}	// namespace FSMfd::Pages
