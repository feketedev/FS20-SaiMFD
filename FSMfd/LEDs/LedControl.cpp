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
		client       { client },
		output       { x52 },
		simvars      { client.CreateVarGroup() },
		leds         { std::move(controllers) },
		stateStamp   { TimePoint::min() },
		nextBlink    { TimePoint::max() },
		enabled      { false }
	{
		DedupSimvarRegister regtor { client, simvars.Group };

		for (LedController& led : leds)
			led.RegisterVariables(regtor);
	}


	LedControl::~LedControl()
	{
		bool cleared = client.TryClearVarGroup(simvars.Group);
		DBG_ASSERT (cleared);

		if (!output.IsConnected())
			return;

		try
		{
			// NOTE: Probably unimportant - these are not the ControlPanel set defaults.
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

		stateStamp = TimePoint::min();
		simvars.Invalidate();
	}


	void LedControl::Enable()
	{
		if (!enabled)
		{
			client.EnableVarGroup(simvars.Group, simvars, UpdateFrequency::FrameDriven);
			enabled = true;
		}
	}


	void LedControl::Disable()
	{
		if (enabled)
		{
			client.DisableVarGroup(simvars.Group);
			simvars.Invalidate();
			enabled = false;
		}
	}


	void LedControl::Blink(TimePoint now)
	{
		if (now < nextBlink || !enabled)
			return;

		SwitchLeds(now, [this](LedController& led, Duration elapsed)
		{
			return led.Blink(output, elapsed);
		});
	}


	void LedControl::ApplyUpdate()
	{
		if (!HasUpdate())
			return;

		const SimvarList& values = simvars.Get();

		SwitchLeds(simvars.LastReceived(), [&](LedController& led, Duration elapsed)
		{
			return led.Update(output, elapsed, values);
		});
	}

	
	template <class LedFun>
	void LedControl::SwitchLeds(TimePoint stamp, LedFun&& update)
	{
		DBG_ASSERT (stateStamp <= stamp);

		const Duration elapsed	= stamp - stateStamp;
		Duration	   tillNext	= Duration::max();

		for (LedController& led : leds)
		{
			Duration hold = update(led, elapsed);
			tillNext = std::min(hold, tillNext);
		}

		// i.e. static color
		bool overflow = stamp > TimePoint::max() - tillNext;

		nextBlink  = overflow ? TimePoint::max() : stamp + tillNext;
		stateStamp = stamp;
	}

	
}	//	namespace FSMfd::Led
