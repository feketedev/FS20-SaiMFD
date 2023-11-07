#pragma once

#include "Utils/BasicUtils.h"
#include "Utils/StringUtils.h"

#include <vector>



namespace FSMfd::Pages
{
	struct SimVarDef;


	class StackableGauge {
	public:
		using ValueList	= Utils::ArraySection<const int32_t>;
		using Display	= Utils::ArraySection<Utils::String::StringSection>;

		const unsigned short			width;
		const unsigned short			height;
		const std::vector<SimVarDef>	variables;


		virtual void Update(const ValueList& measurments, Display&) const = 0;


	  // TODO Interactions
		/// Stack selector scrolled. This gauge is next.
		/// @returns	Gauge has interactable parts and handles further inputs.
		virtual bool Select()	{ return false; }
	  //virtual bool HandleInput() = 0; ?


		virtual ~StackableGauge();

		
		StackableGauge(unsigned short width, unsigned short height, std::vector<SimVarDef>&&);
	};


}	// namespace FSMfd::Pages
