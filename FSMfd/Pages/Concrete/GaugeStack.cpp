#include "GaugeStack.h"

#include "Pages/Gauges/StackableGauge.h"
#include "Utils/Reassignable.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{

#pragma region Construction

	/*static*/ auto GaugeStack::StackGauges(std::vector<std::unique_ptr<StackableGauge>>&& gauges)
	-> std::vector<ActiveGauge>
	{
		std::vector<ActiveGauge> active;
		active.reserve(gauges.size());

		SimClient::VarIdx	nextVar = 0;
		unsigned			x = 0;
		unsigned			y = 0;
		unsigned			lastHeight;

		for (auto& g : gauges)
		{
			bool newRow = 0 < x &&
						  (  lastHeight != g->DisplayHeight
						  || DisplayLength < x + g->DisplayWidth );

			x *= !newRow;
			y += newRow ? lastHeight : 0;

			ActiveGauge gauge { std::move(g), nextVar, x, y };

			nextVar	  += gauge.alg->VarCount();
			x		  += gauge.alg->DisplayWidth;
			lastHeight = gauge.alg->DisplayHeight;

			active.push_back(std::move(gauge));
		}
		return active;
	}


	/*static*/ std::vector<size_t>  GaugeStack::IndexByRow(const std::vector<ActiveGauge>& gauges)
	{
		std::vector<size_t> byRow = { 0 };
		for (size_t i = 0; i < gauges.size(); i++)
		{
			unsigned lastRow = gauges[byRow.back()].posY;
			if (gauges[i].posY > lastRow)
				byRow.push_back(i);
		}
		byRow.push_back(gauges.size());
		return byRow;
	}


	GaugeStack::GaugeStack(uint32_t id, const Dependencies& deps, std::vector<std::unique_ptr<StackableGauge>>&& algs) :
		SimPage  { id, deps },
		gauges   { StackGauges(std::move(algs)) },
		byRow    { IndexByRow(gauges) },
		scroller { *this, TotalHeight() }
	{
		LOGIC_ASSERT (!gauges.empty());

		for (const ActiveGauge& g : gauges)
		{
			for (const SimVarDef& sv : g.alg->Variables)
				RegisterSimVar(sv);
		}

		for (unsigned r = 0; r < RowCount(); r++)
		{
			const ActiveGauge&	rowLast = gauges[byRow[r + 1] - 1];
			const size_t		lineLen = rowLast.posX + rowLast.alg->DisplayWidth;
			const unsigned		lines   = rowLast.alg->DisplayHeight;
			
			for (unsigned l = 0; l < lines; l++)
				scroller.SetLine(l + rowLast.posY, std::wstring (lineLen, L' '));
		}
	}


	unsigned GaugeStack::RowCount() const
	{
		return byRow.size() - 1;
	}


	unsigned GaugeStack::TotalHeight() const
	{
		const ActiveGauge& bottomFirst = gauges[byRow[RowCount() - 1]];
		return bottomFirst.posY + bottomFirst.alg->DisplayHeight;
	}

#pragma endregion




#pragma region Update

	void GaugeStack::CleanContent()
	{
		ModifyDisplayAreas([&](const ActiveGauge& g, StackableGauge::DisplayArea& display) 
		{
			g.alg->Clean(display);
		});
	}


	void GaugeStack::UpdateContent(const SimvarList& simvars)
	{
		ModifyDisplayAreas([&](const ActiveGauge& g, StackableGauge::DisplayArea& display) 
		{
			SimvarSublist measurements { simvars, g.firstVar, g.alg->VarCount() };
			g.alg->Update(measurements, display);
		});
	}


	template <class GaugeDisplayAction>
	void GaugeStack::ModifyDisplayAreas(GaugeDisplayAction&& act)
	{
		using Utils::String::StringSection;

		std::vector<Utils::Reassignable<StringSection>>	area;

		for (unsigned r = 0; r < RowCount(); r++)
		{
			const unsigned rowHeight = gauges[byRow[r]].alg->DisplayHeight;
			area.reserve(rowHeight);

			for (size_t i = byRow[r]; i < byRow[r + 1]; i++)
			{
				const ActiveGauge& g = gauges[i];
				
				area.clear();
				for (unsigned l = 0; l < rowHeight; l++)
				{
					std::wstring& buffer = scroller.ModLine(g.posY + l);
					area.emplace_back(buffer, g.posX, g.alg->DisplayWidth);
				}
				StackableGauge::DisplayArea display { area[0].Ptr(), rowHeight };
				act(g, display);				
			}
		}
	}

#pragma endregion




#pragma region Interaction

	void FSMfd::Pages::GaugeStack::OnScroll(bool up, TimePoint)
	{
		// NOTE: although for interactions probably normal scrolling will be better
		if (up)
			scroller.ScrollDown();
		else
			scroller.ScrollUp();
	}

#pragma endregion


}	// namespace FSMfd::Pages
