#include "LedController.h"

#include "IStateDetector.h"
#include "DirectOutputHelper/X52Output.h"	// colors




namespace FSMfd::Led
{

#pragma region LedOverride

	LedOverride::LedOverride(const LedOverride& src) :
		detector { src.detector->Clone() },
		patterns { src.patterns },
		state    { src.state }
	{
	}


	std::pair<optional<Color>, Duration>	LedOverride::CurrentColor() const
	{
		if (state == Nothing)
			return { Nothing, Duration::max() };

		const BlinkPattern& blink = patterns[*state];
		return { blink.CurrentColor(), blink.HoldTime() };
	}


	void LedOverride::AdvanceBlinking(Duration dur)
	{
		if (state.has_value())
			patterns[*state].Advance(dur);
	}

	
	void LedOverride::Update(Duration sinceBlink, const SimClient::SimvarList& simvars)
	{
		optional<unsigned>	freshState = detector->DetectState(simvars);
		const bool			unchanged  = state == freshState;

		state = freshState;
		
		if (state == Nothing)
			return;

		BlinkPattern& blink = patterns[*state];
		if (unchanged)
			blink.Advance(sinceBlink);
		else
			blink.Reset();
	}

#pragma endregion




#pragma region LedController

	LedController::LedController(UniLed id, bool defaultOn, std::vector<LedOverride> overrides) :
		ledId			  { id },
		isMulticolor	  { false },
		defaultColor	  { defaultOn ? Color::Green : Color::Off },
		overrides		  { std::move(overrides) },
		setColor		  { static_cast<Color>(UINT8_MAX) },
		isCurrentlyStatic { true }
	{
	}


	LedController::LedController(BiLed id, Color defaultCol, std::vector<LedOverride> overrides) :
		ledId			  { id },
		isMulticolor	  { true },
		defaultColor	  { defaultCol },
		overrides		  { std::move(overrides) },
		setColor		  { static_cast<Color>(UINT8_MAX) },  // ControlPanel-set default is unknown
		isCurrentlyStatic { true }							  // ControlPanel-set default at start is static
	{
	}


	void LedController::RegisterVariables(SimClient::DedupSimvarRegister& disp)
	{
		for (LedOverride& ovr : overrides)
			ovr.Detector().RegisterVariables(disp);
	}


	void LedController::ApplyDefault(DOHelper::X52Output& x52)
	{
		Apply(defaultColor, x52);
		isCurrentlyStatic = true;
	}


	void LedController::Apply(Color c, DOHelper::X52Output& x52)
	{
		if (c != setColor && isMulticolor)
			x52.SetColor(static_cast<BiLed>(ledId), c);
		if (c != setColor && !isMulticolor)
			x52.SetState(static_cast<UniLed>(ledId), c != Color::Off);

		setColor = c;
	}


	std::pair<Color, Duration>	LedController::Summarize() const
	{
		Color    winner   = defaultColor;
		Duration holdTime = Duration::max();
		for (const LedOverride& ovr : overrides)
		{
			auto [ovrColor, timeLeft] = ovr.CurrentColor();
			if (ovrColor.has_value())
			{
				winner = *ovrColor;
				holdTime = timeLeft;		// ignore lower prio
			}
			else
			{
				holdTime = std::min(timeLeft, holdTime);
			}
		}
		return { winner, holdTime };
	}



	Duration LedController::Update(DOHelper::X52Output& x52, Duration elapsed, const SimClient::SimvarList& simvars)
	{
		// It's important to always drive all the overrides -> keep potential Blinks ticking in sync
		for (LedOverride& ovr : overrides)
			ovr.Update(elapsed, simvars); 

		auto [color, holdTime] = Summarize();
		Apply(color, x52);

		isCurrentlyStatic = (holdTime == Duration::max());
		return holdTime;
	}

	
	Duration LedController::Blink(DOHelper::X52Output& x52, Duration elapsed)
	{
		if (isCurrentlyStatic)
			return Duration::max();

		for (LedOverride& ovr : overrides)
			ovr.AdvanceBlinking(elapsed);

		auto [color, holdTime] = Summarize();
		Apply(color, x52);

		return holdTime;
	}

#pragma endregion


}	//	namespace FSMfd::Led