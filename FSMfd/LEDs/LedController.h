#pragma once

#include "BlinkPattern.h"
#include "IStateDetector.h"
#include <memory>



namespace FSMfd::Led {

	/// Define color sequence for each state
	using StatePatterns = std::vector<BlinkPattern>;

	/// Define optional static color for each state.
	using StateColors   = std::vector<optional<Color>>;


	// ----------------------------------------------------------------------------------

	/// Can define the Color of a single LED based on SimVars' state, 
	/// or can pass the job to the next Override or Default.
	class LedOverride {
		std::unique_ptr<IStateDetector> 	detector;
		StatePatterns						patterns;
		optional<unsigned>					state;

	public:
		LedOverride(LedOverride&&)		noexcept = default;
		LedOverride(const LedOverride&);

		IStateDetector& Detector()	{ return *detector; }
		
		/// @returns  (actual override, time remain)
		std::pair<optional<Color>, Duration>	CurrentColor() const;

		void Update(Duration sinceBlink, const SimClient::SimvarList&);
		
		void AdvanceBlinking(Duration elapsed);


		template<class States>
		LedOverride(std::unique_ptr<IStateDetector> detector, States patterns) :
			detector { std::move(detector) },
			patterns { ToPatterns(std::move(patterns)) }
		{
		}

		// just sugar
		template<class D, class States>
		LedOverride(D detector, States patterns) :
			detector { new D { std::move(detector) } },
			patterns { ToPatterns(std::move(patterns)) }
		{
		}


	private:
		static StatePatterns ToPatterns(StatePatterns&& p)	 { return std::move(p); }
		static StatePatterns ToPatterns(StateColors&& colors)	
		{
			StatePatterns pat;
			pat.reserve(colors.size());
			for (auto& c : colors)
				pat.emplace_back(c);
			return pat; 
		}
	};



	// ----------------------------------------------------------------------------------

	// TODO: name vs. LedControl?
	/// Governs the state of a single (composite) LED.
	class LedController {
		const uint32_t				ledId;
		const bool					isMulticolor;
		const Color					defaultColor;

		Color						setColor;
		bool						isCurrentlyStatic;		// no blink until next Update
		std::vector<LedOverride>	overrides;

	public:
		LedController(UniLed, bool  defaultOn,	std::vector<LedOverride>);
		LedController(BiLed,  Color defaultCol, std::vector<LedOverride>);

		void RegisterVariables(SimClient::DedupSimvarRegister&);

		void ApplyDefault(DOHelper::X52Output&);

		/// @returns Duration until an upcoming Blink
		Duration Update(DOHelper::X52Output&, Duration elapsed, const SimClient::SimvarList&);
		Duration Blink(DOHelper::X52Output&, Duration elapsed);

	private:
		void Apply(Color, DOHelper::X52Output&);

		std::pair<Color, Duration>	Summarize() const;
	};


}	// namespace FSMfd::Led
