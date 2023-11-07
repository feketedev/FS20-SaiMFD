#pragma once

#include "SimClient/ReceiveBuffer.h"
#include "SimClient/FSClientTypes.h"
#include "Pages/FSPageList.h"
#include "LEDs/LedController.h"



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

	public:
		Configurator(SimClient::FSClient&);

		SimClient::GroupId	SimvarGroup() const  { return configVars.Group; }
		bool				IsReady()	  const  { return configVars.HasData(); }
		
		void Refresh();

		Pages::FSPageList				CreatePages(const Pages::SimPage::Dependencies&) const;
		std::vector<Led::LedController>	CreateLedEffects()								 const;

	private:
		std::vector<Led::LedController> CreateGearEffects() const;
	};


}	//	namespace FSMfd
