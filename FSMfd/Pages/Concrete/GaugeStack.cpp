#include "GaugeStack.h"



namespace FSMfd::Pages
{
	static std::vector<SimVarDef> GetVarDefinitions(const std::vector<const StackableGauge*>&)
	{
		return {};
	}


	GaugeStack::GaugeStack(uint32_t id, FSClient& client, std::vector<const StackableGauge*>&& gauges) :
		SimPage { id, client, GetVarDefinitions(gauges) }
	//	gauges { std::move(gauges) }
	{
		// TODO
	}


	void GaugeStack::CleanContent()
	{
	}

	void GaugeStack::UpdateContent()
	{
	}

}	// namespace FSMfd::Pages
