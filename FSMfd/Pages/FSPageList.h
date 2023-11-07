#pragma once

#include "SimPage.h"
#include <vector>



namespace FSMfd::Pages
{

	class FSPageList {
		const SimPage::Dependencies dependencies;
		
		std::vector<std::unique_ptr<SimPage>>  pages;

		template <class P, class... Args>
		void Add(Args&&...);
		
	public:
		static const unsigned MaxSize;	
		static const unsigned MaxId;	// TODO Del?
		
		FSPageList(const SimPage::Dependencies&);
		void AddAllTo(DOHelper::X52Output&);
	};

}
