#pragma once

#include "LedController.h"
#include "SimClient/ReceiveBuffer.h"
#include "SimClient/FSClientTypes.h"



namespace FSMfd::Led
{

	class LedControl {
		SimClient::FSClient&			client;
		DOHelper::X52Output&			output;
		SimClient::UniqueReceiveBuffer	simvars;
		std::vector<LedController>		leds;

		TimePoint						stateStamp;
		TimePoint						nextBlink;
		bool							enabled;

	public:
		LedControl(DOHelper::X52Output&, SimClient::FSClient&, std::vector<LedController>&&);
		LedControl(LedControl&&) = delete;
		~LedControl();
		
		TimePoint NextBlinkTime() const  { return nextBlink; }
		bool	  HasUpdate()	  const  { return stateStamp < simvars.LastReceived(); }


		// Required because only the active plugin (page) is allowed to drive the LEDs!
		void Enable();
		void Disable();

		void ApplyDefaults();

		/// Change active effects based on last received SimVars. 
		/// Blink unchanged effects as of receive timestamp.
		void ApplyUpdate();

		/// Step active blinking effects appripriately and apply output.
		void Blink(TimePoint now);

	private:
		template <class LedFun>
		void SwitchLeds(TimePoint stamp, LedFun&&);
	};


}	//	namespace FSMfd::Led