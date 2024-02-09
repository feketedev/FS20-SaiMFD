#pragma once

#include "Pages/SimPage.h"
#include "Pages/Scroller.h"
#include "Pages/Gauges/StackableGauge.h"
#include <vector>



namespace FSMfd::Pages
{

	/// Displays a scrollable stack of gauges.
	/// @remarks
	///	  *	No real tiling, just a basic stacking algorithm:
	///		each row's height is defined by the first gauge in it,
	///		which can be followed only by smaller or same height gauges.
	///	  *	Scrolling is multiplied around taller gauges.
	class GaugeStack final : public SimPage {

		struct ActiveGauge {
			std::unique_ptr<StackableGauge>	alg;
			const SimClient::VarIdx			firstVar;
			const unsigned					posX;
			const unsigned					posY;

			unsigned EndX() const;
			unsigned EndY() const;
		};

		std::vector<ActiveGauge>	gauges;
		std::vector<size_t>			byRow;		// gauge row -> 1st gauge; guarded end
		std::vector<size_t>			rowByLine;	// scroller line -> gauge row

		Scroller					scroller;

	public:
		GaugeStack(uint32_t id, const Dependencies&,
				   SimClient::UpdateFrequency = SimClient::UpdateFrequency::PerSecond,
				   std::vector<std::unique_ptr<StackableGauge>>&&			algs = {});


		/// Add a gauge to the bottom.
		/// @param margin: number of blank characters to leave after the previous gauge.
		GaugeStack& Add(std::unique_ptr<StackableGauge>, unsigned margin = 1);

		template<class Gauge>
		GaugeStack& Add(Gauge&&,						 unsigned margin = 1);
		
		template<class ConcreteGauge>
		GaugeStack& Add(std::unique_ptr<ConcreteGauge>,  unsigned margin = 1);		// just in case, disambiguates overload resol.


		void CleanContent()					  override;
		void UpdateContent(const SimvarList&) override;

	private:
		static std::vector<size_t>	IndexByRow(const std::vector<ActiveGauge>&);
		static bool					FitsInRow(const ActiveGauge& prev, unsigned margin, const FSMfd::Pages::StackableGauge&);

		static bool					AddTo(std::vector<ActiveGauge>& gauges, std::unique_ptr<StackableGauge> next, unsigned margin);

		unsigned RowCount()		const;		// Number of gauge rows.
		unsigned TotalHeight()	const;		// Height in screen lines.


		void AllocRowBuffer(unsigned int r);

		template <class GaugeDisplayAction>
		void ModifyDisplayAreas(GaugeDisplayAction&&);

		void OnScroll(bool up, TimePoint) override;
	};



	template<class Gauge>
	GaugeStack&  GaugeStack::Add(Gauge&& next, unsigned margin)
	{
		return Add(std::make_unique<std::remove_reference_t<Gauge>>(std::forward<Gauge>(next)), margin);
	}


	template<class ConcreteGauge>
	GaugeStack&  GaugeStack::Add(std::unique_ptr<ConcreteGauge> next, unsigned margin)
	{
		return Add(std::unique_ptr<StackableGauge> { next.release() }, margin);
	}


}	// namespace FSMfd::Pages

