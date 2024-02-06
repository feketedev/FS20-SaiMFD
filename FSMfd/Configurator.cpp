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
#include "Pages/Gauges/CompactGauge.h"
#include "Pages/Gauges/ColumnGauge.h"
#include "Pages/Gauges/ConditionalGauge.h"
#include "Pages/Gauges/RadioGauge.h"
#include "Pages/Gauges/SwitchGauge.h"
#include "Pages/Gauges/Label.h"
#include "Utils/Debug.h"



namespace FSMfd
{
	using namespace DOHelper;
	using namespace SimClient;
	using namespace Led;
	using namespace Pages;
	using Utils::String::SignUsage;
	using Utils::String::DecimalUsage;


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
		client.AddVar(configVars.Group, { "SPOILER AVAILABLE",	 "Bool" });
		client.AddVar(configVars.Group, { "FLAPS AVAILABLE",	 "Bool" });
		client.AddVar(configVars.Group, { "AUTOPILOT AVAILABLE", "Bool" });
	}


	void Configurator::Refresh()
	{
		Debug::Info("Configurator", "Refreshing. Old data existed: ", configVars.HasData());
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


	bool Configurator::HasSpoilers() const
	{
		return configVars.Get()[4].AsUnsigned32() > 0;
	}


	bool Configurator::HasFlaps() const
	{
		return configVars.Get()[5].AsUnsigned32() > 0;
	}


	bool Configurator::HasAutopilot() const
	{
		return configVars.Get()[6].AsUnsigned32() > 0;
	}

#pragma endregion




#pragma region Page Config

	constexpr unsigned DisplayLength = SimPage::DisplayLength;


	FSPageList Configurator::CreatePages(const SimPage::Dependencies& deps) const
	{
		FSPageList pages { deps };
		
		AddBaseInstruments(pages);
		AddConfigInstruments(pages);
		AddEnginesMonitor(pages);
		if (HasAutopilot())
			AddAutopilotSettings(pages);
		AddRadioFreqSettings(pages);
		AddRadioNavPage(pages);

		return pages;
	}

	
	void Configurator::AddBaseInstruments(FSPageList& pages) const
	{
		std::vector<DisplayVar> baseVars {
			{ L"Accelr: ",	SimVarDef { "G FORCE",					"number",			RequestType::Real }, L"g  " },
			{ L"IAS:",		SimVarDef { "AIRSPEED INDICATED",		"knots"				}},
			{ L"TAS:",		SimVarDef { "AIRSPEED TRUE",			"knots"				}},
			{ L"Mach:",		SimVarDef { "AIRSPEED MACH",			"mach",				RequestType::Real }, L"   "	},
			{ L"VSpd:",		SimVarDef { "VERTICAL SPEED",			"feet per minute",	RequestType::SignedInt }, SignUsage::PrependPlus },
			{ L"Alt:",		SimVarDef { "PLANE ALTITUDE",			"feet"				},					 L"ft " },
			{ L"Outside:",	SimVarDef { "AMBIENT TEMPERATURE",		"celsius",			RequestType::Real }, DecimalUsage { 1 } },
			{ L"Press: ",	SimVarDef { "AMBIENT PRESSURE",			"millibar",			RequestType::Real }, L"mB" },
			{ L"Dyn.P: ",	SimVarDef { "DYNAMIC PRESSURE",			"millibar",			RequestType::Real }, L"mB" },
		};

		pages.Add<ReadoutScrollList>(std::move(baseVars));
	};


	void Configurator::AddConfigInstruments(FSPageList& pages) const
	{
		GaugeStack& page = pages.Add<GaugeStack>();

		if (HasSpoilers())
		{
			// MAYBE: allow for Align::Left
			page.Add(CompactGauge { 10, { L"SPOIL ",{ "SPOILERS LEFT POSITION",  "percent" } }});
			page.Add(CompactGauge { 5,  { L"-",		{ "SPOILERS RIGHT POSITION", "percent" } }});
		}

		if (HasFlaps())
		{
			page.Add(CompactGauge { 10, { L" LEFs",	{ "LEADING EDGE FLAPS LEFT ANGLE",  "degrees" }} });
			page.Add(CompactGauge { 5,  { L"-",		{ "LEADING EDGE FLAPS RIGHT ANGLE", "degrees" }} });

			page.Add(CompactGauge { 10, { L"Flaps",	{ "TRAILING EDGE FLAPS LEFT ANGLE",  "degrees" }} });
			page.Add(CompactGauge { 5,  { L"-",		{ "TRAILING EDGE FLAPS RIGHT ANGLE", "degrees" }} });
			page.Add(CompactGauge { 11, { L"",		{ "FLAPS HANDLE INDEX",  "number" }} });
			page.Add(CompactGauge { 2,  { L"/",		{ "FLAPS NUM HANDLE POSITIONS",  "number" }} }, 0);
		}

		page.Add(ColumnGauge { L"Stab%", {{ "ELEVATOR TRIM PCT", "percent", RequestType::Real }},		SignUsage::PrependPlus });
		page.Add(ColumnGauge { L" Ail%", {{ "AILERON TRIM PCT",  "percent", RequestType::SignedInt }},	SignUsage::PrependPlus });
		page.Add(ColumnGauge { L"Rud%",	 {{ "RUDDER TRIM PCT",   "percent", RequestType::Real }},		{ 1, SignUsage::PrependPlus }});
	}


	void Configurator::AddAutopilotSettings(Pages::FSPageList& pages) const
	{
		GaugeStack& panel = pages.Add<GaugeStack>(UpdateFrequency::OnValueChange);

		const SwitchGauge::SideSymbols dotRight   { Nothing, L'·' };
		const SwitchGauge::SideSymbols dotsAround { L'·', L'·' };

		panel.Add(SwitchGauge { L"AT", "AUTOPILOT THROTTLE ARM",			dotsAround });
		panel.Add(SwitchGauge { L"AP", "AUTOPILOT MASTER",					dotsAround }, 2);
		panel.Add(SwitchGauge { L"FD", "AUTOPILOT FLIGHT DIRECTOR ACTIVE",	dotsAround }, 2);

		// 2nd row
		// NOTE: Would be nice to utilize DedupSimvarRegister here, but it's not worth the effort for this only use case.
		panel.Add(ConditionalGauge {{ "AUTOPILOT AIRSPEED HOLD", "AUTOPILOT MACH HOLD" }, {
			std::make_unique<SwitchGauge>(L"IAS", "AUTOPILOT AIRSPEED HOLD", dotRight),
			std::make_unique<SwitchGauge>(L"Mch", "AUTOPILOT MACH HOLD",	 dotRight)
		}});
		panel.Add(ConditionalGauge {{ "AUTOPILOT AIRSPEED HOLD", "AUTOPILOT MACH HOLD" }, {
			std::make_unique<CompactGauge>(3, SimVarDef  { "AUTOPILOT AIRSPEED HOLD VAR", "knots" }),
			std::make_unique<CompactGauge>(3, DisplayVar { L".", { "AUTOPILOT MACH HOLD VAR", "percent" }, L"" })
		}}, 0);
		panel.Add(SwitchGauge  { L"VS", "AUTOPILOT VERTICAL HOLD", dotRight });
		panel.Add(CompactGauge { 5, DisplayVar { L"", { "AUTOPILOT VERTICAL HOLD VAR", "ft/min", RequestType::SignedInt }, L"" }}, 0);
 
		// 3rd row
		panel.Add(SwitchGauge  { L"HDG", "AUTOPILOT HEADING LOCK",	dotRight });
		panel.Add(CompactGauge { 3, { "AUTOPILOT HEADING LOCK DIR", "degrees" } }, 0);
		panel.Add(SwitchGauge  { L"Al", "AUTOPILOT ALTITUDE LOCK",	dotRight });
		panel.Add(CompactGauge { 5, { "AUTOPILOT ALTITUDE LOCK VAR", "ft" } }, 0);
	}


	void Configurator::AddEnginesMonitor(FSPageList& pages) const
	{
		if (EngineCount() == 0)		// glider
			return;

		GaugeStack& engStack = pages.Add<GaugeStack>();

		if (EngineCount() == 1 && !IsJet())
		{
			engStack.Add(CompactGauge { 9, DisplayVar { L"Prop", { "PROP BETA:1",			"degrees",	RequestType::SignedInt }}, false });
			engStack.Add(CompactGauge { 7, DisplayVar { L"OiP ", { "ENG OIL PRESSURE:1",	"psi" },								L"" } }, 0);
			engStack.Add(CompactGauge { 8, DisplayVar { L"RPM ", { "PROP RPM:1",			"RPM" }} });
			engStack.Add(CompactGauge { 7, DisplayVar { L"OiT",	 { "ENG OIL TEMPERATURE:1",	"celsius" },							L"" } });
			if (IsPiston())
			{
				engStack.Add(CompactGauge { 8, DisplayVar { L"Mix ",	{ "RECIP MIXTURE RATIO:1",			"percent", RequestType::Real }} });
				engStack.Add(CompactGauge { 7, DisplayVar { L"FF ",		{ "ENG FUEL FLOW GPH:1",			"gallons per hour" },				 L"" } });
				engStack.Add(CompactGauge { 8, DisplayVar { L"CarbT ",	{ "RECIP CARBURETOR TEMPERATURE:1",	"celsius", RequestType::SignedInt }, L"" } });
			}
			else
			{
				engStack.Add(CompactGauge { 8, DisplayVar { L"N1",		{ "TURB ENG N1:1",					"percent" }} });
				engStack.Add(CompactGauge { 7, DisplayVar { L"FF ",		{ "RECIP ENG FUEL FLOW:1",			"pounds per hour" }, L"" } });
				engStack.Add(CompactGauge { 8, DisplayVar { L"TQ ",		{ "ENG TORQUE:1",					"Foot pounds" },	 L"" } });
			}
			engStack.Add(CompactGauge { 7, DisplayVar { L"Vac ", { "SUCTION PRESSURE",		"inHg",		RequestType::Real }, 1,	L"" } });
			if (IsTurboprop())
				engStack.Add(CompactGauge { 9, DisplayVar { L"ITT",		{ "TURB ENG ITT:1",					"celsius" }}, true });

			engStack.Add(SwitchGauge { L" Pump", "GENERAL ENG FUEL PUMP ON:1", { Nothing, L'*' }});

			return;
		}

		// -- Jet or Multi-engine --

		std::vector<DisplayVar> engVars;
		if (IsJet())
		{
			engVars.insert(engVars.end(), {
				{ L"REVR ",	{ "TURB ENG REVERSE NOZZLE PERCENT:",	"percent",		RequestType::Real }},
				{ L"EGT ",	{ "ENG EXHAUST GAS TEMPERATURE:",		"celsius"		}},
				{ L"N1 ",	{ "TURB ENG N1:",						"percent",		RequestType::Real }, 1 },
				{ L"N2 ",	{ "TURB ENG N2:",						"percent",		RequestType::Real }, 1 },
				{ L"RPM",	{ "GENERAL ENG RPM:",					"RPM"			}},
			});
		}
		if (IsTurboprop() || IsPiston())
		{
			engVars.insert(engVars.end(), {
				{ L"EGT ",	 { "ENG EXHAUST GAS TEMPERATURE:",		"celsius"		}},
				{ L"Prop",	 { "PROP BETA:",						"degrees",		RequestType::SignedInt }},
				{ L"PrpRPM", { "PROP RPM:",							"RPM"			}},
				{ L"EngRPM", { "GENERAL ENG RPM:",					"RPM"			}},
			});
		}
		if (IsTurboprop())
		{
			engVars.insert(engVars.end(), {
				{ L"TQ ",	{ "ENG TORQUE:",						"foot pounds"	}},
				{ L"N1 ",	{ "TURB ENG N1:",						"percent",		RequestType::Real }, 1 },
				{ L"N2 ",	{ "TURB ENG N2:",						"percent",		RequestType::Real }, 1 },
				{ L"ITT ",	{ "TURB ENG ITT:",						"celsius"		}},
			});
		};
		if (IsPiston())
		{
			engVars.insert(engVars.end(), {
				{ L"Mix ",	{ "RECIP MIXTURE RATIO:",				"percent",		RequestType::Real }},
				{ L"MAP ",	{ "RECIP ENG MANIFOLD PRESSURE:",		"inHg",			RequestType::Real }, L"inH" },
				{ L"Cowl ",	{ "RECIP ENG COWL FLAP POSITION:",			"percent"		}},
				{ L"Carb",	{ "RECIP CARBURETOR TEMPERATURE:",		"celsius",		RequestType::SignedInt }},
				{ L"Cyl ",	{ "RECIP ENG CYLINDER HEAD TEMPERATURE:", "celsius",		RequestType::SignedInt }},
			});
		}

		SimVarDef fuelFlow = IsPiston()
			? SimVarDef { "ENG FUEL FLOW GPH:", "gallons per hour" }
			: SimVarDef { "ENG FUEL FLOW PPH:",	"pounds per hour"  };

		// common for multi-engine
		engVars.insert(engVars.end(), {
			{ L"FF ",	fuelFlow },
			{ L"OIL ",	{ "ENG OIL PRESSURE:",					"psi",				  RequestType::Real }, 1 },
			{ L"OIL ",	{ "ENG OIL TEMPERATURE:",				"celsius",			  RequestType::Real }, 1 },
		//	{ L"OIL%",	{ "ENG OIL QUANTITY:",					"percent",			  RequestType::Real }, 1 },	// always 100%
		});

		for (const DisplayVar& dv : engVars)
			engStack.Add(EnginesGauge { EngineCount(), dv });

		// extra on/off indicators
		if (IsPiston() || IsTurboprop())
		{
			// MAYBE: This is hacky -> SwitchGauge to support 2 vars? Magnetos?
			engStack.Add(SwitchGauge { L"    ", "GENERAL ENG FUEL PUMP ON:1", { Nothing, L'*' }});
			engStack.Add(SwitchGauge { L"Pump", "GENERAL ENG FUEL PUMP ON:2", { Nothing, L'*' }}, 0);
		}
	}


	void Configurator::AddRadioFreqSettings(Pages::FSPageList& pages) const
	{
		GaugeStack& radioStack = pages.Add<GaugeStack>(UpdateFrequency::OnValueChange);

		radioStack.Add(Label { L"ACT  -COM-  STBY" });
		radioStack.Add(RadioGauge { "COM", 1 });
		radioStack.Add(RadioGauge { "COM", 2 });
		radioStack.Add(CompactGauge { 5, DisplayVar { L"",  { "COM ACTIVE FREQ IDENT:1", "", RequestType::String }} });
		radioStack.Add(CompactGauge { 5, DisplayVar { L"/", { "COM ACTIVE FREQ IDENT:2", "", RequestType::String }} }, 0);

		radioStack.Add(Label { L"ACT  -NAV-  STBY" });
		radioStack.Add(RadioGauge { "NAV", 1 });
		radioStack.Add(RadioGauge { "NAV", 2 });
		radioStack.Add(Label { L"ACT  -ADF-  STBY" });
		radioStack.Add(RadioGauge { "ADF", 1 });
		radioStack.Add(RadioGauge { "ADF", 2 });
		radioStack.Add(CompactGauge { 11, DisplayVar { L"XPDR:", { "TRANSPONDER CODE:1", "Number" }} });
	}


	void Configurator::AddRadioNavPage(Pages::FSPageList& pages) const
	{
		GaugeStack& navStack = pages.Add<GaugeStack>();
		
		navStack.Add(CompactGauge { 4, DisplayVar { L"",  { "NAV IDENT:1", "",				 RequestType::String }} });
		navStack.Add(Label { L"-NAV-" });
		navStack.Add(CompactGauge { 4, DisplayVar { L"",  { "NAV IDENT:2", "",				 RequestType::String	}} });
		navStack.Add(CompactGauge { 5, DisplayVar { L"»", { "NAV OBS:1",		  "degrees", RequestType::SignedInt }} });
		navStack.Add(CompactGauge { 5, DisplayVar { L"»", { "NAV OBS:2",		  "degrees", RequestType::SignedInt }} }, 6);
		navStack.Add(CompactGauge { 5, DisplayVar { L"",  { "NAV RADIAL ERROR:1", "degrees", RequestType::SignedInt }, SignUsage::PrependPlus } });
		navStack.Add(CompactGauge { 5, DisplayVar { L"",  { "NAV RADIAL ERROR:2", "degrees", RequestType::SignedInt }, SignUsage::PrependPlus } }, 6);
		navStack.Add(CompactGauge { 7, DisplayVar { L"",  { "NAV DME:1",   "nautical miles", RequestType::Real		}, SignUsage::ForbidNegativeValues } });
		navStack.Add(CompactGauge { 7, DisplayVar { L"",  { "NAV DME:2",   "nautical miles", RequestType::Real		}, SignUsage::ForbidNegativeValues } }, 2);

		navStack.Add(CompactGauge { 4, DisplayVar { L"", { "ADF IDENT:1", "",		  RequestType::String	 }} });
		navStack.Add(Label { L"-ADF-" });																	 
		navStack.Add(CompactGauge { 4, DisplayVar { L"", { "ADF IDENT:2", "",		  RequestType::String	 }} });
		navStack.Add(CompactGauge { 5, DisplayVar { L"", { "ADF RADIAL:1", "degrees", RequestType::SignedInt }, SignUsage::PrependPlus } });
		navStack.Add(CompactGauge { 5, DisplayVar { L"", { "ADF RADIAL:2", "degrees", RequestType::SignedInt }, SignUsage::PrependPlus } }, 4);
		navStack.Add(CompactGauge { 7, DisplayVar { L"", { "ADF SIGNAL:1", "decibel", RequestType::Real		 },	SignUsage::ForbidNegativeValues } });
		navStack.Add(CompactGauge { 7, DisplayVar { L"", { "ADF SIGNAL:2", "decibel", RequestType::Real		 },	SignUsage::ForbidNegativeValues } }, 2);
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



	template <class... Elems>
	std::vector<LedOverride> IfAvail(const Elems&... elems)
	{
		std::vector<LedOverride> res;
		
		auto addValue = [&res](auto& o)
		{
			if constexpr (std::is_same_v<std::decay_t<decltype(o)>, optional<LedOverride>>)
			{
				if (o.has_value())
					res.push_back(*o);
			}
			else
			{
				res.push_back(o);
			}
		};
		(..., addValue(elems));
		return res;
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
		optional<LedOverride> spoilersUnArmed;
		if (HasSpoilers() && HasRetractableGears())
		{
			spoilersUnArmed.emplace(
				Conditional {
					SwitchDetector	{ "GEAR HANDLE POSITION" },
					SwitchDetector	{ "SPOILERS ARMED" },
				},
				StateColors { Color::Amber, Nothing }
			);
		};

		// triggering for Min(left, right): asymm spoilers used along with aileron to roll should not trigger
		optional<LedOverride> spoilersExtended;
		if (HasSpoilers())
		{
			spoilersExtended.emplace(
				Max { SwitchDetector { "SPOILERS LEFT POSITION",  "percent" },
					  SwitchDetector { "SPOILERS RIGHT POSITION", "percent" } },
				StatePatterns { { Nothing },
								{{ Nothing, 800ms }, { Color::Amber, 200ms }} }
			);
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
			LedController { ButtonE, Color::Green, IfAvail(spoilersUnArmed, spoilersExtended, overspdBlink, stallBlink) },
			LedController { Fire,     true,		   { overspdBlink, stallBlink } },
			LedController { ButtonA, Color::Off,   { flapsExtended1 } },
			LedController { ButtonB, Color::Off,   { flapsExtended2 } },
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


		optional<LedOverride> steeringLockWarning;
		if (IsTaildragger())						// Nosewheellock does not seem to be present anywhere
		{
			steeringLockWarning.emplace(
				Conditional {
					gearNotRetracted,
					SwitchDetector { IsTaildragger() ? "TAILWHEEL LOCK ON" : "NOSEWHEEL LOCK ON" }
				},
				StateColors { LedColor::Amber, Nothing }
			);
		}

		LedOverride parkBrakeWarning {
			Conditional {
				gearNotRetracted,
				SwitchDetector	{ "BRAKE PARKING POSITION" }
			},
			StateColors		{ Nothing, LedColor::Amber }
		};


		if (HasRetractableGears())
		{
			return { 
				{ Toggle1, Color::Amber, { leftGearState, parkBrakeWarning } },
				{ Toggle2, Color::Amber, IfAvail(centerGearState, steeringLockWarning) },
				{ Toggle3, Color::Amber, { rightGearState } },
			};
		}
		else
		{
			return {
				{ Toggle1, Color::Off, { parkBrakeWarning } },
				{ Toggle2, Color::Off, IfAvail(steeringLockWarning) },
				{ Toggle3, Color::Off, {} },
			};
		}
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
		optional<LedOverride> jetReverserActive;
		if (IsJet())
		{
			jetReverserActive.emplace(
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
			);
		}


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


		return {
			LedController { Pov2,	 Color::Off, { apMaster, apOnGlideslope, apDisengage } },
			LedController { ButtonD, Color::Off, { atArmed, atActive, apDisengage } },
			LedController { ButtonI, Color::Off, IfAvail(generalReverseThrust, jetReverserActive, atTOGA) },
		};
	}

#pragma endregion


}
