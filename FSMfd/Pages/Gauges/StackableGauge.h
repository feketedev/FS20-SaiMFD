#pragma once

#include "FSMfdTypes.h"
#include "SimVarDef.h"
#include "Utils/BasicUtils.h"
#include "Utils/StringUtils.h"
#include <vector>



namespace FSMfd::Pages
{
	/// Virtual list of own Variables. 
	/// Tiny utility to avoid additional allocations during Updates.
	class SimvarSublist {
		const SimClient::SimvarList&	list;
		const SimClient::VarIdx			start;
	
	public:
		const SimClient::VarIdx			VarCount;

		SimClient::SimvarValue operator[](SimClient::VarIdx)								const;
		SimvarSublist		   Sublist(SimClient::VarIdx relFirst, SimClient::VarIdx len)	const;

		SimvarSublist(const SimClient::SimvarList&, SimClient::VarIdx start, SimClient::VarIdx len);
	};



	/// Display for 1 or more related values on a Page.
	class StackableGauge {
	public:
		using DisplayArea = Utils::ArraySection<Utils::String::StringSection>;

		const unsigned					DisplayWidth;
		const unsigned					DisplayHeight;
		const std::vector<SimVarDef>	Variables;

		auto VarCount() const	{ return Practically<SimClient::VarIdx>(Variables.size()); }

		virtual void Clean(DisplayArea&)						= 0;
		virtual void Update(const SimvarSublist&, DisplayArea&) = 0;

		virtual ~StackableGauge();

	protected:
		StackableGauge(unsigned width, unsigned height, std::vector<SimVarDef>&&);
	};


}	// namespace FSMfd::Pages
