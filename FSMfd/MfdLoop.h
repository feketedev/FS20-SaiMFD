#pragma once

	/*  Part of FS20-SaiMFD			   *
	 *  Copyright 2023 Norbert Fekete  *
     *                                 *
	 *  Released under GPLv3.		   */


#include "DirectOutputHelper/DOHelperTypes.h"
#include "SimClient/FSClientTypes.h"
#include "SimClient/IReceiver.h"
#include "FSMfdTypes.h"



namespace FSMfd
{
	class MfdLoop final : public SimClient::IEventReceiver {
		using X52Output = DOHelper::X52Output;
		using FSClient  = SimClient::FSClient;

		const volatile bool&			 uninterrupted;
		const SimClient::FSTypeMapping&  typeMapping;

		bool							 aircraftChanged = false;
		bool							 inFlight		 = false;
		SimClient::NotificationCode		 codeAircraftLoaded;
		SimClient::NotificationCode		 codeInFlight;
		
	public:
		const char* const FSClientName;

		Duration WelcomeAnimationIval = 1s;

		Duration BasePeriod      = 1s;		// ~ SIMCONNECT_PERIOD_SECOND for data updates
		unsigned AnimationFreq   = 2;		// Local Page-logic (e.g. blink)
		unsigned FSPollFreq      = 6;		// Receive data or events from FS
		unsigned UpdateFreq      = 1;		// Present received data on MFD
		Duration HotReceiveDelay = 50ms;	// Try to responsively receive data after input or Page-change

		// TODO Maybe: struct Timings { Duration a,b,c } --> Freqs

		MfdLoop(const volatile bool& uninterruptedFlag, const char* fSClientName, const SimClient::FSTypeMapping&);
		void Run(X52Output&);

	private:
		FSClient ConnectToFS(X52Output&);
		void	 WaitForFlight(X52Output&, FSClient&, const Configurator& config);
		void	 PollFS(X52Output&, Led::LedControl&, FSClient&);

		bool CanUse(X52Output&)		const;
		bool CanFlyAircraft(FSClient&)		const;
		bool CanReload(FSClient&)	const;

		// IEventReceiver
		void Notify(SimClient::NotificationCode code, uint32_t parameter, TimePoint) override;
		void Notify(SimClient::NotificationCode, const char* string, TimePoint)		 override;
	};


}	//namespace FSMfd
