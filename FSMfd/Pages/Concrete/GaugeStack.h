#pragma once

#include "Pages/SimPage.h"
#include "Pages/Scroller.h"
#include "Pages/Gauges/StackableGauge.h"
#include <vector>



namespace FSMfd::Pages
{
	class GaugeStack : public SimPage
	{
		struct ActiveGauge {
			std::unique_ptr<StackableGauge>	alg;
			const SimClient::VarIdx			firstVar;
			const unsigned					posX;
			const unsigned					posY;
		};

		const std::vector<ActiveGauge>	gauges;
		const std::vector<size_t>		byRow;		// gauge row -> 1st gauge; guarded end

		Scroller	scroller;

	public:
		GaugeStack(uint32_t id, const Dependencies&, std::vector<std::unique_ptr<StackableGauge>>&& algs);

		void CleanContent()					  override;
		void UpdateContent(const SimvarList&) override;

	private:
		static std::vector<ActiveGauge>  StackGauges(std::vector<std::unique_ptr<StackableGauge>>&&);
		static std::vector<size_t>		 IndexByRow(const std::vector<ActiveGauge>&);

		unsigned RowCount()		const;
		unsigned TotalHeight()	const;

		template <class GaugeDisplayAction>
		void ModifyDisplayAreas(GaugeDisplayAction&&);

		void OnScroll(bool up, TimePoint) override;
	};


}	// namespace FSMfd::Pages

