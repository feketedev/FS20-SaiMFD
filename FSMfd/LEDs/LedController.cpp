#include "LedController.h"

#include "IStateDetector.h"
#include "DirectOutputHelper/X52Output.h"	// colors




namespace FSMfd::Led
{
	LedOverride::LedOverride(const LedOverride& src) :
		detector  { src.detector->Clone() },
		patterns  { src.patterns },
		lastState { src.lastState }
	{
	}


	optional<Color> LedOverride::CalcResult(const SimClient::SimvarList& simvars)
	{
		optional<unsigned> state = detector->DeterminState(simvars);
		if (!state.has_value())
		{
			lastState = UINT_MAX;
			return Nothing;
		}

		if (*state != lastState && lastState < patterns.size())
			patterns[lastState].Reset();

		lastState = *state;
		BlinkPattern& blink = patterns[*state];

		blink.Step(1);					// TODO ticks?
		return blink.CurrentColor();
	}




	LedController::LedController(UniLed id, bool defaultOn, std::vector<LedOverride> overrides) :
		ledId        { id },
		isMulticolor { false },
		defaultColor { defaultOn ? Color::Green : Color::Off },
		overrides    { std::move(overrides) },
		lastColor	 { static_cast<Color>(UINT8_MAX) }
	{
	}


	LedController::LedController(BiLed id, Color defaultCol, std::vector<LedOverride> overrides) :
		ledId        { id },
		isMulticolor { true },
		defaultColor { defaultCol },
		overrides    { std::move(overrides) },
		lastColor	 { static_cast<Color>(UINT8_MAX) }
	{
	}


	void LedController::RegisterVariables(SimClient::DedupSimvarRegister& disp)
	{
		for (LedOverride& ovr : overrides)
			ovr.Detector().RegisterVariables(disp);
	}


	void LedController::ApplyDefault(DOHelper::X52Output& x52)
	{
		if (isMulticolor)
			x52.SetColor(static_cast<BiLed>(ledId), defaultColor);
		else
			x52.SetState(static_cast<UniLed>(ledId), defaultColor != Color::Off);

		lastColor = defaultColor;
	}


	void LedController::Update(DOHelper::X52Output& x52, const SimClient::SimvarList& simvars)
	{
		// It's important to always drive all the overrides -> keep Blinks ticking consistently
		optional<Color> upmost;
		for (LedOverride& ovr : overrides)
		{
			optional<Color> curr = ovr.CalcResult(simvars);
			if (curr.has_value())
				upmost = *curr;
		}

		Color ultimate = upmost.value_or(defaultColor);

		if (ultimate != lastColor && isMulticolor)
			x52.SetColor(static_cast<BiLed>(ledId), ultimate);
		if (ultimate != lastColor && !isMulticolor)
			x52.SetState(static_cast<UniLed>(ledId), ultimate != Color::Off);

		lastColor = ultimate;
	}



}	//	namespace FSMfd::Led