#include "FSPageList.h"

#include "DirectOutputHelper/X52Output.h"
#include "Utils/Debug.h"

#include "Concrete/ReadoutScrollList.h"
#include "Concrete/GaugeStack.h"
#include "Gauges/EnginesGauge.h"



namespace FSMfd::Pages {
	



	// ---- Page configurations ----

	static const std::vector<DisplayVar> airspeedVars {
		{ L"SPOIL:",	SimVarDef { "SPOILERS LEFT POSITION",	"Percent", RequestType::Real }, L""	},
		{ L"AT ARM:",	SimVarDef { "AUTOPILOT THROTTLE ARM",	"Bool" }, L""	},
		{ L"AT SPEED:",	SimVarDef { "AUTOPILOT AIRSPEED HOLD",	"Bool" }, L""	},

		{ L"IAS:",	SimVarDef { "AIRSPEED INDICATED",	"knots"					  }, L"kts"	},
		{ L"TAS:",	SimVarDef { "AIRSPEED TRUE",		"knots"					  }, L"kts"	},
		{ L"Mach:",	SimVarDef { "AIRSPEED MACH",		"mach", RequestType::Real }, L""	},
		{ L"Alt:",	SimVarDef { "PLANE ALTITUDE",		"ft"					  }, L"ft"	}
	};


	static const std::vector<DisplayVar> engVars {
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



	// ---- Page List ----

	constexpr unsigned FSPageList::MaxSize = 2;
	constexpr unsigned FSPageList::MaxId   = MaxSize + 1;

	FSPageList::FSPageList(const SimPage::Dependencies& deps) : dependencies { deps }
	{
		std::vector<std::unique_ptr<StackableGauge>> engineGauges;

		// TODO: move properly to Configurator
		for (const DisplayVar& dv : engVars)
			engineGauges.push_back(std::make_unique<EnginesGauge>(2, dv));

		Add<ReadoutScrollList>(airspeedVars);
		Add<GaugeStack>(std::move(engineGauges));

		LOGIC_ASSERT (pages.size() <= MaxSize);
	}


	template<class P, class... Args>
	void FSPageList::Add(Args&&... args)
	{
		uint32_t id = pages.size() + 1;
		pages.push_back(
			std::make_unique<P>(id, dependencies, std::forward<Args>(args)...)
		);
	}


	void FSPageList::AddAllTo(DOHelper::X52Output& device)
	{
		bool activate = true;
		for (auto& p : pages)
		{
			device.AddPage(*p, activate);
			activate = false;
		}
	}


}	// namespace FSMfd::Pages