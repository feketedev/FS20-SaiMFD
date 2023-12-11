#pragma once

#include "Pages/SimPage.h"
#include "Pages/Scroller.h"
#include "Pages/SimvarPrinter.h"



namespace FSMfd::Pages
{

	class ReadoutScrollList : public SimPage {
		
		std::vector<std::pair<DisplayVar, SimvarPrinter>>	variables;
		
		Scroller	scroller;
		

		void CleanContent()					  override;
		void UpdateContent(const SimvarList&) override;

		void OnScroll(bool up, TimePoint) override;

	public:
		ReadoutScrollList(uint32_t id, const Dependencies&, std::vector<DisplayVar>);
	};


}	// namespace FSMfd::Pages
