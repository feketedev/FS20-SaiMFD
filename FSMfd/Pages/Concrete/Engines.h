#pragma once

#include "Pages/SimPage.h"
#include "Pages/Scroller.h"
#include "Utils/StringUtils.h"




namespace FSMfd::Pages
{

	class Engines : public SimPage {
	public:
		struct DisplayVar {
			const char* name;
			const char* unit;
			const std::wstring_view	text;
		};

	private:
		std::vector<DisplayVar>		simvars;
		Scroller					scroller;

		virtual void CleanContent()  override;
		virtual void UpdateContent(const SimvarList&) override;

		virtual void OnScroll(bool up, TimePoint) override;

	public:
		Engines(uint32_t id, SimClient::FSClient&, std::vector<DisplayVar>);
	};

}	// namespace FSMfd::Pages