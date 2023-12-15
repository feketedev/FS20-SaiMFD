#pragma once

#include "StackableGauge.h"



namespace FSMfd::Pages
{

	class Label final : public StackableGauge {
		std::wstring text;

	public:
		Label(std::wstring text);

		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;
	};


}	// namespace FSMfd::Pages
