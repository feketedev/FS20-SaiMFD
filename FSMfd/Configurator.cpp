#include "Configurator.h"

	/*  Currently no configuration via files implemented.   *
	 *  This file is intended to allow configuration in a   *
	 *  hardcoded, but flexible and readable manner.		*
	 *													    *
	 *  Part of FS20-SaiMFD	 Copyright 2023 Norbert Fekete  *
	 *  Released under GPLv3.							    */


#include "SimClient/FSClient.h"
#include "LEDs/LedController.h"
#include "LEDs/StateDetectors.h"
#include "Pages/Concrete/GaugeStack.h"
#include "Pages/Concrete/ReadoutScrollList.h"
#include "Pages/Gauges/EnginesGauge.h"
#include "Utils/Debug.h"



namespace FSMfd
{
	using namespace DOHelper;
	using namespace SimClient;
	using namespace Led;
	using namespace Pages;


#pragma region Decision SimVars

	Configurator::Configurator(SimClient::FSClient& client) :
		client	   { client },
		configVars { client.CreateVarGroup(GroupLifetime::Permanent) }
	{
		LOGIC_ASSERT (client.IsConnected());

		client.AddVar(configVars.Group, { "IS GEAR RETRACTABLE", "Bool" });
		client.AddVar(configVars.Group, { "IS TAIL DRAGGER",	 "Bool" });
		client.AddVar(configVars.Group, { "NUMBER OF ENGINES",	 "Number" });
		client.AddVar(configVars.Group, { "ENGINE TYPE",		 "Enum" });

		client.RequestOnetimeUpdate(configVars.Group, configVars);
	}


	void Configurator::Refresh()
	{
		Debug::Info("Config INVALIDATED");
		configVars.Invalidate();
		client.RequestOnetimeUpdate(configVars.Group, configVars);
	}


	bool Configurator::HasRetractableGears() const
	{
		return configVars.Get()[0].AsUnsigned32();
	}


	bool Configurator::IsTaildragger() const
	{
		return configVars.Get()[1].AsUnsigned32();
	}


	unsigned Configurator::EngineCount() const
	{
		return configVars.Get()[2].AsUnsigned32();
	}


	bool Configurator::IsJet() const
	{
		return configVars.Get()[3].AsUnsigned32() == 1;
	}


	bool Configurator::IsPiston() const
	{
		return configVars.Get()[3].AsUnsigned32() == 0;
	}


	bool Configurator::IsTurboprop() const
	{
		return configVars.Get()[3].AsUnsigned32() == 5;
	}


	bool Configurator::IsHeli() const
	{
		return configVars.Get()[3].AsUnsigned32() == 3;
	}

#pragma endregion




#pragma region Page Config

	FSPageList Configurator::CreatePages(const SimPage::Dependencies& deps) const
	{
		FSPageList pages { deps };
		
		AddBaseInstruments(pages);
		AddConfigInstruments(pages);
		AddEnginesPage(pages);

		return pages;
	}

	
	void Configurator::AddBaseInstruments(FSPageList& pages) const
	{
		std::vector<DisplayVar> baseVars {
			{ L"IAS:",	SimVarDef { "AIRSPEED INDICATED",	"knots"					  }, L"kts"	},
			{ L"TAS:",	SimVarDef { "AIRSPEED TRUE",		"knots"					  }, L"kts"	},
			{ L"Mach:",	SimVarDef { "AIRSPEED MACH",		"mach", RequestType::Real }, L""	},
			{ L"Alt:",	SimVarDef { "PLANE ALTITUDE",		"ft"					  }, L"ft"	},
		};

		pages.Add<ReadoutScrollList>(std::move(baseVars));
	};


	void Configurator::AddConfigInstruments(FSPageList& pages) const
	{
		std::vector<DisplayVar> configVars {
			// TODO: Left-Right
			{ L"SPOIL:",	SimVarDef { "SPOILERS LEFT POSITION",	"Percent", RequestType::Real }, L""	},
			{ L"AT ARM:",	SimVarDef { "AUTOPILOT THROTTLE ARM",	"Bool" }, L""	},
			{ L"AT SPEED:",	SimVarDef { "AUTOPILOT AIRSPEED HOLD",	"Bool" }, L""	},
		};

		pages.Add<ReadoutScrollList>(std::move(configVars));
	}


	// TODO: differentiate for propeller and single-engine aircraft
	void Configurator::AddEnginesPage(FSPageList& pages) const
	{
		if (EngineCount() == 0)		// glider
			return;

		const std::vector<DisplayVar> engVars {
			{ L"REV",   SimVarDef { "TURB ENG REVERSE NOZZLE PERCENT:",	"percent", RequestType::Real  } },
			{ L"EGT",   SimVarDef { "ENG EXHAUST GAS TEMPERATURE:",		"celsius" } },
			{ L"RPM",   SimVarDef { "GENERAL ENG RPM:",					"RPM" } },
			{ L"N1 %",  SimVarDef { "TURB ENG N1:",						"percent",			RequestType::Real }, 1 },
			{ L"N2 %",  SimVarDef { "TURB ENG N2:",						"percent",			RequestType::Real }, 1 },
			{ L"FF",    SimVarDef { "ENG FUEL FLOW PPH:",				"pounds per hour" } },
			{ L"OILP",  SimVarDef { "ENG OIL PRESSURE:",				"psi",				RequestType::Real }, 1 },
			{ L"OILT",  SimVarDef { "ENG OIL TEMPERATURE:",				"celsius",			RequestType::Real }, 1 },
		//	{ L"OIL%",   SimVarDef { "ENG OIL QUANTITY:",				"percent",			RequestType::Real }, 1 },	// always 100%
		};

		std::vector<std::unique_ptr<StackableGauge>> engineGauges;
		for (const DisplayVar& dv : engVars)
			engineGauges.push_back(std::make_unique<EnginesGauge>(EngineCount(), dv));

		pages.Add<GaugeStack>(std::move(engineGauges));
	}

#pragma endregion




#pragma region LED Config


	std::vector<LedController> Configurator::CreateLedEffects() const
	{
		LOGIC_ASSERT (IsReady());

		std::vector<LedController> leds = CreateGenericWarningEffects();
		Utils::Append(leds, CreateGearEffects());
		Utils::Append(leds, CreateEngApEffects());
		
		return leds;
	}


	std::vector<LedController> Configurator::CreateGenericWarningEffects() const
	{
		LedOverride stallBlink {
			SwitchDetector	{ "STALL WARNING" },
			StatePatterns	{
				{ Nothing },
				{{ Color::Red, 200ms }, { Color::Off, 200ms }} 
			}
		};

		LedOverride overspdBlink {
			SwitchDetector	{ "OVERSPEED WARNING" },
			StatePatterns	{ 
				{ Nothing },
				{{ Color::Red, 800ms }, { Color::Off, 200ms }}
			}
		};
		
		// going to land => arm spoilers
		LedOverride spoilersUnArmed {
			Conditional {
				SwitchDetector	{ "GEAR HANDLE POSITION" },
				SwitchDetector	{ "SPOILERS ARMED" },
			},
			StateColors { Color::Amber, Nothing }
		};

		// triggering for Min(left, right): asymm spoilers used along with aileron to roll should not trigger
		LedOverride spoilersExtended {
			Max { SwitchDetector { "SPOILERS LEFT POSITION",  "percent" },
				  SwitchDetector { "SPOILERS RIGHT POSITION", "percent" } },
			StatePatterns { { Nothing },
							{{ Nothing, 800ms }, { Color::Amber, 200ms }} }
		};

		LedOverride flapsExtended1 {
			Max { RangeDetector { "TRAILING EDGE FLAPS LEFT PERCENT",  "percent", Boundaries { UpIncluding(0), 50, 100 } },
				  RangeDetector { "TRAILING EDGE FLAPS RIGHT PERCENT", "percent", Boundaries { UpIncluding(0), 50, 100 } } },
			StateColors { Nothing, Color::Green, Color::Amber, Color::Off }
		};
		LedOverride flapsExtended2 {
			Max { RangeDetector { "TRAILING EDGE FLAPS LEFT PERCENT",  "percent", Boundaries { 25, 75 } },
				  RangeDetector { "TRAILING EDGE FLAPS RIGHT PERCENT", "percent", Boundaries { 25, 75 } } },
			StateColors { Nothing, Color::Green, Color::Amber }
		};


		return {
			LedController { Throttle, true,		   { overspdBlink } },
			LedController { ButtonE, Color::Green, { spoilersUnArmed, spoilersExtended, overspdBlink, stallBlink } },
			LedController { Fire,     true,		   { overspdBlink, stallBlink } },
			LedController { ButtonA, Color::Off, { flapsExtended1 } },
			LedController { ButtonB, Color::Off, { flapsExtended2 } },
		};
	}


	std::vector<LedController> Configurator::CreateGearEffects() const
	{
		SwitchDetector gearNotRetracted { "GEAR TOTAL PCT EXTENDED", "percent" };

		LedOverride leftGearState {
			RangeDetector	{ "GEAR LEFT POSITION", "percent", Boundaries { UpIncluding(0), 100 } },
			StateColors		{ LedColor::Off, LedColor::Red, LedColor::Green }
		};

		LedOverride centerGearState {
			RangeDetector	{ "GEAR CENTER POSITION", "percent", Boundaries { UpIncluding(0), 100 } },
			StateColors		{ LedColor::Off, LedColor::Red, LedColor::Green }
		};

		LedOverride rightGearState {
			RangeDetector	{ "GEAR LEFT POSITION", "percent", Boundaries { UpIncluding(0), 100 } },
			StateColors		{ LedColor::Off, LedColor::Red, LedColor::Green }
		};


		LedOverride steeringLockWarning {
			Conditional {
				gearNotRetracted,
				SwitchDetector { IsTaildragger() ? "TAILWHEEL LOCK ON" : "NOSEWHEEL LOCK ON" }
			},
			StateColors		{ LedColor::Amber, Nothing }
		};

		LedOverride parkBrakeWarning {
			Conditional {
				gearNotRetracted,
				SwitchDetector	{ "BRAKE PARKING POSITION" }
			},
			StateColors		{ Nothing, LedColor::Amber }
		};


		if (!HasRetractableGears())
		{
			return {
				{ Toggle1, Color::Green, {} },
				{ Toggle2, Color::Green, { steeringLockWarning } },
				{ Toggle3, Color::Green, { parkBrakeWarning } },
			};
		}

		if (IsTaildragger())
		{
			return { 
				{ Toggle1, Color::Amber, { leftGearState } },
				{ Toggle2, Color::Amber, { centerGearState, steeringLockWarning } },
				{ Toggle3, Color::Amber, { rightGearState, parkBrakeWarning } },
			};
		}

		// Rretractable Tricycle
		// - Nosewheellock does not seem to be present anywhere
		return { 
			{ Toggle1, Color::Amber, { leftGearState } },
			{ Toggle2, Color::Amber, { centerGearState /*, steeringLockWarning*/ } },
			{ Toggle3, Color::Amber, { rightGearState, parkBrakeWarning } },
		};
	}


	std::vector<LedController> Configurator::CreateEngApEffects() const
	{
		// this is purely the control axis mode, not an indication of actual thrust-reverser status
		// however, from a control perspective this is the more useful
		LedOverride generalReverseThrust {
			// TODO: Unused variables receive 0, but should consider EngineCount instead of spamming...
			Max {
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:1" },
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:2" },
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:3" },
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:4" },
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:5" },
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:6" },
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:7" },
				SwitchDetector { "GENERAL ENG REVERSE THRUST ENGAGED:8" }
			},
			StateColors { Color::Green, Color::Red }
		};

		// this shows when reverse thrust is actually applied
		// -> approximation: e.g. 787 gives around 0.4% reverse thrust with nozzles just opened
		LedOverride jetReverserActive {
			Max {
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:1", "percent", Boundaries { 0.2 } },
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:2", "percent", Boundaries { 0.2 } },
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:3", "percent", Boundaries { 0.2 } },
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:4", "percent", Boundaries { 0.2 } },
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:5", "percent", Boundaries { 0.2 } },
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:6", "percent", Boundaries { 0.2 } },
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:7", "percent", Boundaries { 0.2 } },
				RangeDetector { "TURB ENG REVERSE NOZZLE PERCENT:8", "percent", Boundaries { 0.2 } }
			},
			StatePatterns { { Nothing }, {{ Color::Off, 500ms }, { Color::Red, 500ms }} }
		};


		LedOverride apMaster {
			SwitchDetector	{ "AUTOPILOT MASTER" },
			StateColors		{ Color::Off, Color::Green }
		};

		LedOverride apDisengage {
			SwitchDetector	{ "AUTOPILOT DISENGAGED" },
			StateColors		{ Nothing, Color::Red }
		};
		
		LedOverride apOnGlideslope {
			SwitchDetector	{ "AUTOPILOT GLIDESLOPE ACTIVE" },
			StateColors		{ Nothing, Color::Amber }
		};
		
		LedOverride atTOGA {
			SwitchDetector	{ "AUTOPILOT TAKEOFF POWER ACTIVE" },
			StatePatterns	{ { Nothing },
							  {{ Color::Off, 500ms }, { Color::Green, 500ms }} }
		};

		LedOverride atArmed {
			SwitchDetector	{ "AUTOPILOT THROTTLE ARM" },
			StateColors		{ Nothing, Color::Amber }
		};

		LedOverride atActive {
			SwitchDetector	{ "AUTOPILOT MANAGED THROTTLE ACTIVE" },
			StateColors		{ Nothing, Color::Green }
		};


		auto thrustRelated = IsJet() 
			? std::vector<LedOverride> { generalReverseThrust, atTOGA }
			: std::vector<LedOverride> { generalReverseThrust, jetReverserActive, atTOGA };

		return {
			LedController { Pov2,	 Color::Off, { apMaster, apOnGlideslope, apDisengage } },
			LedController { ButtonD, Color::Off, { atArmed, atActive, apDisengage } },
			LedController { ButtonI, Color::Off,  std::move(thrustRelated) },
		};
	}

#pragma endregion


}