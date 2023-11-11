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
#include "Utils/Debug.h"



namespace FSMfd
{
	using namespace DOHelper;
	using namespace SimClient;
	using namespace Led;


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

#pragma endregion




#pragma region Page Config

	//Pages::FSPageList Configurator::CreatePages(const Pages::SimPage::Dependencies& deps) const
	//{
	//	return {};
	//}

#pragma endregion




#pragma region LED Config


	std::vector<Led::LedController> Configurator::CreateLedEffects() const
	{
		LOGIC_ASSERT (IsReady());

		LedOverride stallBlink {
			SwitchDetector	{ "STALL WARNING" },
			StatePatterns	{
				{ Nothing },
				{{ LedColor::Red, 1 }, { Nothing, 1 }} 
			}
		};

		LedOverride overspdBlink {
			SwitchDetector	{ "OVERSPEED WARNING" },
			StatePatterns	{ 
				{ Nothing },
				{{ LedColor::Red, 3 }, { Nothing, 2 }}
			}
		};

		std::vector<LedController> leds = CreateGearEffects();
		
		leds.push_back({ ButtonE, Color::Green, { stallBlink, overspdBlink } });
		leds.push_back({ ButtonA, Color::Green, { stallBlink, overspdBlink } });
		
		return leds;
	}


	std::vector<Led::LedController> Configurator::CreateGearEffects() const
	{
		LOGIC_ASSERT (IsReady());

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

#pragma endregion


}