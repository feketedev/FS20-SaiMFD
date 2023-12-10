#pragma once

#include "StackableGauge.h"
#include <array>
#include <memory>



namespace FSMfd::Pages
{

	class ConditionalGauge : public StackableGauge
	{
		/*const*/ std::array<std::unique_ptr<StackableGauge>, 2>	gauges;
		const std::array<std::string, 2>							triggerVarNames;
		const std::array<SimClient::VarIdx, 2>						varPositions;
		unsigned													active = 0;

		bool SelectActive(const SimvarSublist& state);

	public:
		ConditionalGauge(std::array<std::string, 2> triggerVarNames, std::array<std::unique_ptr<StackableGauge>, 2>);
		ConditionalGauge(ConditionalGauge&&) = default;

		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;
	};


}	// namespace FSMfd::Pages
