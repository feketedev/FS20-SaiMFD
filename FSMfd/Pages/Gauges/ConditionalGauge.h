#pragma once

#include "StackableGauge.h"
#include <array>
#include <memory>



namespace FSMfd::Pages
{

	/// Present one or another gauge on the same area, depending on respective triggers.
	/// Currently implements 2 choices, but extensible if needed.
	class ConditionalGauge final : public StackableGauge {

		/*const*/ std::array<std::unique_ptr<StackableGauge>, 2>	gauges;
		const std::array<std::string, 2>							triggerVarNames;		// last "" => default choice
		const std::array<SimClient::VarIdx, 2>						varPositions;			// for gauges respectively
		unsigned													active			 = 0;

		bool SelectActive(const SimvarSublist& state);

		bool HasDefaultChoice() const		   { return triggerVarNames.back().empty(); }


	public:
		// 1st true => gauge 1; 2nd true => gauge 2; else no change
		ConditionalGauge(std::array<std::string_view, 2> triggerVarNames, std::array<std::unique_ptr<StackableGauge>, 2>);

		// true => gauge 1; false => gauge 2
		ConditionalGauge(std::string_view				conditionVarName, std::array<std::unique_ptr<StackableGauge>, 2>);

		ConditionalGauge(ConditionalGauge&&) = default;

		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;
	};


}	// namespace FSMfd::Pages
