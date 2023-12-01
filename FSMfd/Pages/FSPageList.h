#pragma once

#include "SimPage.h"
#include <vector>



namespace FSMfd::Pages
{

	class FSPageList {
		const SimPage::Dependencies				dependencies;

		uint32_t								nextId;
		std::vector<std::unique_ptr<SimPage>>	pages;

	public:
		const auto& Pages() const				{ return pages; }

		
		FSPageList(const SimPage::Dependencies& deps, uint32_t firstId = 1) :
			dependencies { deps },
			nextId		 { firstId }
		{
		}
		

		template <class P, class... Args>
		P&	Add(Args&&... args)
		{
			uint32_t id = nextId++;
			pages.push_back(
				std::make_unique<P>(id, dependencies, std::forward<Args>(args)...)
			);
			return static_cast<P&>(*pages.back());
		}
	};

}
