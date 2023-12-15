#include "ConditionalGauge.h"
#include "SimClient/IReceiver.h"
#include "Utils/Debug.h"
#include <type_traits>



namespace FSMfd::Pages
{
	template <class T>
	using ChoiceList = std::array<T, 2>;


	static unsigned MaxWidth(const ChoiceList<std::unique_ptr<StackableGauge>>& gauges)
	{
		return std::max(gauges[0]->DisplayWidth, gauges[1]->DisplayWidth);
	}


	static unsigned MaxHeight(const ChoiceList<std::unique_ptr<StackableGauge>>& gauges)
	{
		return std::max(gauges[0]->DisplayHeight, gauges[1]->DisplayHeight);
	}


	static std::vector<SimVarDef> SummarizeSimvars(const ChoiceList<std::string>&						triggers,
												   const ChoiceList<std::unique_ptr<StackableGauge>>&	gauges)
	{
		std::vector<SimVarDef> vars;
		vars.reserve(triggers.size() + gauges.size());

		for (const auto& name : triggers)
			vars.push_back({ name, "Bool" });
		for (const auto& g : gauges)
			Utils::Append(vars, g->Variables);

		return vars;
	}


	static ChoiceList<SimClient::VarIdx>  CalcVarPositions(const ChoiceList<std::unique_ptr<StackableGauge>>& gauges)
	{
		constexpr SimClient::VarIdx triggerCount = std::tuple_size_v<std::decay_t<decltype(gauges)>>;

		ChoiceList<SimClient::VarIdx> res;

		SimClient::VarIdx pos = triggerCount;
		for (unsigned i = 0; i < gauges.size(); i++)
		{
			res[i] = pos;
			pos += gauges[i]->VarCount();
		}
		return res;
	}


	ConditionalGauge::ConditionalGauge(ChoiceList<std::string> triggerVarNames, ChoiceList<std::unique_ptr<StackableGauge>> gauges) :
		StackableGauge  { MaxWidth(gauges), MaxHeight(gauges), { SummarizeSimvars(triggerVarNames, gauges) } },
		gauges          { std::move(gauges) },
		triggerVarNames { std::move(triggerVarNames) },
		varPositions	{ CalcVarPositions(this->gauges) }
	{
	}


	bool ConditionalGauge::SelectActive(const SimvarSublist& state)
	{
		unsigned i = 0;
		while (i < triggerVarNames.size() && !state[i].AsUnsigned32())
			i++;

		if (i == active || i >= triggerVarNames.size())
			return false;

		active = i;
		return true;
	}


	void ConditionalGauge::Clean(DisplayArea& display)
	{
		gauges[active]->Clean(display);
	}


	void ConditionalGauge::Update(const SimvarSublist& state, DisplayArea& display)
	{
		if (SelectActive(state))
		{
			for (unsigned i = 0; i < DisplayHeight; i++)
				display[i].FillWith(L' ');

			gauges[active]->Clean(display);
		}

		SimvarSublist substate = state.Sublist(varPositions[active], gauges[active]->VarCount());
		gauges[active]->Update(substate, display);
	}


}	// namespace FSMfd::Pages
