#pragma once

#include "StackableGauge.h"
#include "Pages/SimvarPrinter.h"
#include <array>



namespace FSMfd::Pages
{

	class EnginesGauge : public StackableGauge
	{
		struct Field;

		const std::wstring			title;
		const std::vector<Field>	layout;
		const SimvarPrinter			printValue;
		
		unsigned EngCount() const	{ return Variables.size(); }

		static std::vector<Field>	CreateLayout(unsigned engCount, unsigned titleLen);

	public:
		EnginesGauge(unsigned engineCount, const DisplayVar& varProto);

		EnginesGauge(EnginesGauge&&);
		EnginesGauge(const EnginesGauge&);
		~EnginesGauge();

		void Clean(DisplayArea&)						const override;
		void Update(const SimvarSublist&, DisplayArea&) const override;
	};

	
}	// namespace FSMfd::Pages
