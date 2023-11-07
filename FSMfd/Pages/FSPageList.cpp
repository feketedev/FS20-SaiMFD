#include "FSPageList.h"

#include "DirectOutputHelper/X52Output.h"
#include "Utils/Debug.h"


#include "Concrete/ReadoutScrollList.h"
//#include "Concrete/Engines.h"



namespace FSMfd::Pages {
	



	// ---- Page configurations ----

	static const std::vector<DisplayVar> airspeedVars {
		{ L"IAS:",	SimVarDef { "AIRSPEED INDICATED",	"knots"					  }, L"kts"	},
		{ L"TAS:",	SimVarDef { "AIRSPEED TRUE",		"knots"					  }, L"kts"	},
		{ L"Mach:",	SimVarDef {  "AIRSPEED MACH",		"mach", RequestType::Real }, L""	},
		{ L"Alt:",	SimVarDef { "PLANE ALTITUDE",		"ft"					  }, L"ft"	}
	};


	//static const std::vector<Engines::DisplayVar> engVars {
	//	{ "ENG EXHAUST GAS TEMPERATURE:",	"Rankine",	L"EGT" },
	//	{ "GENERAL ENG RPM:",				"RPM",		L"RPM" },
	//	{ "TURB ENG N1:",					"N1%",		L"N1%" },
	//	{ "TURB ENG N2:",					"N2%",		L"N2%" }
	//};


	// ---- Page List ----

	constexpr unsigned FSPageList::MaxSize = 2;
	constexpr unsigned FSPageList::MaxId   = MaxSize + 1;

	FSPageList::FSPageList(const SimPage::Dependencies& deps) : dependencies { deps }
	{
		Add<ReadoutScrollList>(airspeedVars);
//		Add<Engines>(engVars);

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