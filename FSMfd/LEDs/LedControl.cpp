#include "LedControl.h"

#include "SimClient/FSClient.h"
#include "SimClient/DedupSimvarRegister.h"
#include "DirectOutputHelper/X52Output.h"
#include "Utils/Debug.h"



namespace FSMfd::Led
{
	using namespace SimClient;
	using DOHelper::X52Output;


	LedControl::LedControl(X52Output& x52, FSClient& client, std::vector<LedController>&& controllers) :
		client		{ client },
		output      { x52 },
		simvars     { client.CreateVarGroup() },
		leds        { std::move(controllers) },
		lastUpdated { TimePoint::min() }
	{
		DedupSimvarRegister regtor { client, simvars.Group };

		for (LedController& led : leds)
			led.RegisterVariables(regtor);

		client.EnableVarGroup(simvars.Group, simvars, /*fastUpdate:*/ true);
	}


	LedControl::~LedControl()
	{
		bool cleared = client.TryClearVarGroup(simvars.Group);
		DBG_ASSERT (cleared);

		if (!output.IsConnected())
			return;

		try
		{
			ApplyDefaults();
		}
		catch (...)
		{
			DBG_BREAK_M ("Failed to restore default LED colors.");
		}
	}


	void LedControl::ApplyDefaults()
	{
		for (LedController& led : leds)
			led.ApplyDefault(output);
	}


	bool LedControl::HasUpdate()
	{
		return lastUpdated < simvars.LastReceived();
	}


	void LedControl::ApplyUpdate()
	{
		if (!HasUpdate())	// TODO: ASSERT?
			return;

		for (LedController& led : leds)
			led.Update(output, simvars.Get());
	}


}	//	namespace FSMfd::Led