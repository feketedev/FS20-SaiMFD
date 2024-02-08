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


	static std::vector<SimVarDef> SummarizeSimvars(const ChoiceList<std::string_view>&					triggers,
												   const ChoiceList<std::unique_ptr<StackableGauge>>&	gauges)
	{
		std::vector<SimVarDef> vars;
		vars.reserve(triggers.size() + gauges.size());

		for (const auto& name : triggers)
		{
			if (name.empty())
			{
				LOGIC_ASSERT_M (&name == &triggers.back(), "Only the default option can be empty.");
				break;
			}
			vars.push_back({ std::string { name }, "Bool" });
		}
		for (const auto& g : gauges)
			Utils::Append(vars, g->Variables);

		return vars;
	}


	static ChoiceList<SimClient::VarIdx>  CalcVarPositions(unsigned triggerVarCount, const ChoiceList<std::unique_ptr<StackableGauge>>& gauges)
	{
		ChoiceList<SimClient::VarIdx> res;

		SimClient::VarIdx pos = triggerVarCount;
		for (unsigned i = 0; i < gauges.size(); i++)
		{
			res[i] = pos;
			pos += gauges[i]->VarCount();
		}
		return res;
	}


	// no conversion ctor?? :/
	static ChoiceList<std::string>	CopyToStrings(const ChoiceList<std::string_view>& list)
	{
		ChoiceList<std::string> res;
		for (unsigned i = 0; i < res.size(); i++)
			res[i] = list[i];
		return res;
	}


	ConditionalGauge::ConditionalGauge(ChoiceList<std::string_view> triggerVarNames, ChoiceList<std::unique_ptr<StackableGauge>> gauges) :
		StackableGauge  { MaxWidth(gauges), MaxHeight(gauges), { SummarizeSimvars(triggerVarNames, gauges) } },
		gauges          { std::move(gauges) },
		triggerVarNames { CopyToStrings(triggerVarNames) },
		varPositions	{ CalcVarPositions(Implied<unsigned>(triggerVarNames.size()), this->gauges) }
	{
	}


	ConditionalGauge::ConditionalGauge(std::string_view conditionVarName, ChoiceList<std::unique_ptr<StackableGauge>> gauges) :
		StackableGauge  { MaxWidth(gauges), MaxHeight(gauges), { SummarizeSimvars({ conditionVarName }, gauges)}},
		gauges          { std::move(gauges) },
		triggerVarNames { std::string { conditionVarName }, "" },
		varPositions	{ CalcVarPositions(1, this->gauges) }
	{
	}


	bool ConditionalGauge::SelectActive(const SimvarSublist& state)
	{
		constexpr unsigned choices  = std::tuple_size_v<decltype(triggerVarNames)>;
		const	  unsigned triggers = choices - HasDefaultChoice();

		unsigned i = 0;
		while (i < triggers && !state[i].AsUnsigned32())
			i++;

		if (i == active || i >= choices)
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
