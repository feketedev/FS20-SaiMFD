#pragma once

#include "StackableGauge.h"



namespace FSMfd::Pages
{
	class RadioGauge : public StackableGauge {

		void (&printFreq) (unsigned, Utils::String::StringSection);

	public:
		RadioGauge(const char* radioType, uint8_t index);

		void Clean(DisplayArea&)						override;
		void Update(const SimvarSublist&, DisplayArea&)	override;

	};


}	// namespace FSMfd::Pages