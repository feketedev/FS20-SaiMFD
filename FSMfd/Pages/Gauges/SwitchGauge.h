#pragma once

#include "StackableGauge.h"



namespace FSMfd::Pages
{

	class SwitchGauge : public StackableGauge
	{
		std::wstring		title;
		optional<wchar_t>	leftSymbol;
		optional<wchar_t>	rightSymbol;

		Utils::String::StringSection	GetTitleField(DisplayArea&) const;

	public:
		using SideSymbols = std::pair<optional<wchar_t>, optional<wchar_t>>;

		SwitchGauge(std::wstring title, std::string simvarName, SideSymbols);


		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;
	};


}	// namespace FSMfd::Pages
