#include "GaugeStack.h"

#include "Pages/Gauges/StackableGauge.h"
#include "Utils/Reassignable.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{

#pragma region Assemble

	/*static*/ bool GaugeStack::FitsInRow(const ActiveGauge& prev, unsigned margin, const StackableGauge& g)
	{
		// no smart tiling, just allow smaller gauge to follow a taller one
		return prev.alg->DisplayHeight >= g.DisplayHeight
			&& prev.EndX() + margin + g.DisplayWidth <= SimPage::DisplayLength;
	}


	/*static*/ std::vector<size_t>  GaugeStack::IndexByRow(const std::vector<ActiveGauge>& gauges)
	{
		std::vector<size_t> byRow = { 0 };
		if (gauges.empty())
			return byRow;

		for (size_t i = 0; i < gauges.size(); i++)
		{
			unsigned lastRow = gauges[byRow.back()].posY;
			if (gauges[i].posY > lastRow)
				byRow.push_back(i);
		}
		byRow.push_back(gauges.size());

		return byRow;
	}


	GaugeStack::GaugeStack(uint32_t id, const Dependencies& deps, SimClient::UpdateFrequency freq, std::vector<std::unique_ptr<StackableGauge>>&& algs) :
		SimPage  { id, deps },
		scroller { *this }
	{
		SetUpdateFrequency(freq);
		if (freq == SimClient::UpdateFrequency::OnValueChange)
			BackgroundReceiveEnabled = true;

		gauges.reserve(algs.size());
		for (std::unique_ptr<StackableGauge>& a : algs)
			AddTo(gauges, std::move(a), 1u);

		byRow = IndexByRow(gauges);
		DBG_ASSERT (!byRow.empty());	// guarded end

		scroller.EnsureLineCount(TotalHeight());
		rowByLine.reserve(TotalHeight());
		for (unsigned r = 0; r < RowCount(); r++)
		{
			AllocRowBuffer(r);
			for (unsigned li = 0; li < gauges[byRow[r]].alg->DisplayHeight; li++)
				rowByLine.push_back(r);
		}
		DBG_ASSERT (rowByLine.size() == TotalHeight());

		for (const ActiveGauge& g : gauges)
		{
			for (const SimVarDef& v : g.alg->Variables)
				RegisterSimVar(v);
		}
	}
	

	GaugeStack&  GaugeStack::Add(std::unique_ptr<StackableGauge> next, unsigned margin)
	{
		DBG_ASSERT (byRow.back() == gauges.size());

		// register first: this way a SimConnect exception won't corrupt this objects state
		// (shouldn't build on that though)
		for (const SimVarDef& v : next->Variables)
			RegisterSimVar(v);

		const unsigned height = next->DisplayHeight;

		bool newRow = AddTo(gauges, std::move(next), margin);
		if (newRow)
		{
			const unsigned row = RowCount();
			byRow.push_back(gauges.size());
			for (unsigned i = 0; i < height; i++)
				rowByLine.push_back(row);

			scroller.EnsureLineCount(TotalHeight());
		}
		else
		{
			byRow.back() = gauges.size();
		}
		LOGIC_ASSERT (scroller.LineCount() == TotalHeight());
		AllocRowBuffer(RowCount() - 1);

		return *this;
	}


	/// @returns:  Needed new row.
	/*static*/ bool GaugeStack::AddTo(std::vector<ActiveGauge>& gauges, std::unique_ptr<StackableGauge> next, unsigned margin)
	{
		if (gauges.empty())
		{
			gauges.push_back({ std::move(next), 0, 0, 0 });
			return true;
		}

		const ActiveGauge&		last	= gauges.back();
		const SimClient::VarIdx nextVar = last.firstVar + last.alg->VarCount();
		const bool				newRow  = !FitsInRow(last, margin, *next);

		unsigned x = last.EndX() + margin;
		unsigned y = last.posY;
		if (newRow)
		{
			unsigned iHead = Practically<unsigned>(gauges.size());
			while (0 < iHead && gauges[iHead - 1].posY == last.posY)
				--iHead;

			x =  0;
			y += gauges[iHead].alg->DisplayHeight;
		}
		gauges.push_back({ std::move(next), nextVar, x, y });
		return newRow;
	}


	void GaugeStack::AllocRowBuffer(unsigned int r)
	{
		DBG_ASSERT_M (r + 1 < byRow.size(), "Index out of date.");

		const ActiveGauge& rowLast = gauges[byRow[r + 1] - 1];
		const size_t	   lineLen = rowLast.EndX();
		const unsigned	   lines   = rowLast.alg->DisplayHeight;

		for (unsigned l = 0; l < lines; l++)
			scroller.SetLine(l + rowLast.posY, std::wstring (lineLen, L' '));
	}


	unsigned GaugeStack::RowCount() const
	{
		return Practically<unsigned>(byRow.size()) - 1;
	}


	unsigned GaugeStack::TotalHeight() const
	{
		if (gauges.empty())
			return 0;

		const ActiveGauge& bottomFirst = gauges[byRow[RowCount() - 1]];
		return bottomFirst.EndY();
	}


	unsigned GaugeStack::ActiveGauge::EndX() const
	{
		return posX + alg->DisplayWidth;
	}

	unsigned GaugeStack::ActiveGauge::EndY() const
	{
		return posY + alg->DisplayHeight;
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

	void GaugeStack::OnScroll(bool up, TimePoint)
	{
		if (gauges.empty())
			return;

		// NOTE: inverted scrolling - although for interactions probably normal scrolling will be better
		// NOTE: scrolling to reach top/bottom of next gauge, as long as no lines get skipped
		if (up)
		{
			bool firstDown = scroller.IsAtop();
			scroller.ScrollDown();
			const ActiveGauge& bottomGauge = gauges[byRow[rowByLine[scroller.LastDisplayedLine()]]];
			if (bottomGauge.EndY() > scroller.LastDisplayedLine() + 1)
				scroller.ScrollDown();
			if (!firstDown && bottomGauge.EndY() > scroller.LastDisplayedLine() + 1)
				scroller.ScrollDown();
		}
		else
		{
			bool firstUp = scroller.IsBottom();
			scroller.ScrollUp();
			const ActiveGauge& topGauge = gauges[byRow[rowByLine[scroller.FirstDisplayedLine()]]];
			if (topGauge.posY < scroller.FirstDisplayedLine())
				scroller.ScrollUp();
			if (!firstUp && topGauge.posY < scroller.FirstDisplayedLine())
				scroller.ScrollUp();
		}
	}

#pragma endregion


}	// namespace FSMfd::Pages
