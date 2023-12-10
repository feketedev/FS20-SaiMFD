#pragma once

	/*  Part of FS20-SaiMFD			   *
	 *  Copyright 2023 Norbert Fekete  *
     *                                 *
	 *  Released under GPLv3.		   */


#include "LEDs/LedController.h"
#include "Pages/FSPageList.h"
#include "Pages/SimPage.h"
#include "SimClient/FSClientTypes.h"
#include "SimClient/ReceiveBuffer.h"



namespace FSMfd 
{

	class Configurator {
		SimClient::FSClient&			client;
		SimClient::UniqueReceiveBuffer	configVars;

		// these are simly hardcoded for Page/LedControl creation decisions
		bool		HasRetractableGears() const;	// var 0
		bool		IsTaildragger()		  const;	// var 1

		unsigned	EngineCount()	const;			// var 2
		
		bool		IsJet()			const;			//
		bool		IsPiston()		const;			// var 3 
		bool		IsTurboprop()	const;			// 
		bool		IsHeli()		const;			// 

		bool		HasSpoilers()	const;			// var 4

	public:
		Configurator(SimClient::FSClient&);

		SimClient::GroupId	SimvarGroup() const  { return configVars.Group; }
		bool				IsReady()	  const  { return configVars.HasData(); }
		
		void Refresh();

		Pages::FSPageList				CreatePages(const Pages::SimPage::Dependencies&) const;
		std::vector<Led::LedController>	CreateLedEffects()								 const;

	private:
		void AddBaseInstruments(Pages::FSPageList&)		const;
		void AddConfigInstruments(Pages::FSPageList&)	const;
		void AddAutopilotSettings(Pages::FSPageList&)	const;
		void AddEnginesMonitor(Pages::FSPageList&)		const;
		void AddRadioFreqSettings(Pages::FSPageList&)	const;


		std::vector<Led::LedController> CreateGenericWarningEffects()	const;
		std::vector<Led::LedController> CreateGearEffects()				const;
		std::vector<Led::LedController> CreateEngApEffects()			const;
	};


}	//	namespace FSMfd
