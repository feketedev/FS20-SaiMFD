#pragma once

#include "Pages/SimPage.h"

#include <vector>



namespace FSMfd::Pages
{
	class StackableGauge;

	class GaugeStack : public SimPage
	{
		struct ActiveGauge {
			StackableGauge&	alg;					// TODO: owned or just pointed?
			uint32_t		firstVariable;
			size_t			firstLine;
			size_t			firstColumn;
		};

		std::vector<ActiveGauge>	gauges;

	public:
		GaugeStack(uint32_t id, FSClient&,		std::vector<const StackableGauge*>&&);

		virtual void CleanContent()  override;
		virtual void UpdateContent() override;
	};


}	// namespace FSMfd::Pages

