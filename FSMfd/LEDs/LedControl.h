#pragma once

#include "LedController.h"
#include "SimClient/ReceiveBuffer.h"
#include "SimClient/FSClientTypes.h"



namespace FSMfd::Led
{

	class LedControl {
		SimClient::FSClient&			client;	// TODO into Buffer?
		DOHelper::X52Output&			output;
		SimClient::UniqueReceiveBuffer	simvars;
		std::vector<LedController>		leds;
		TimePoint						lastUpdated;

	public:
		LedControl(DOHelper::X52Output&, SimClient::FSClient&, std::vector<LedController>&&);
		LedControl(LedControl&&) = delete;
		~LedControl();


		void ApplyDefaults();
		bool HasUpdate();
		void ApplyUpdate();
	};


}	//	namespace FSMfd::Led