#pragma once

#include "BlinkPattern.h"
#include "IStateDetector.h"
#include <memory>



namespace FSMfd::Led {

	/// Define color sequence for each state
	using StatePatterns = std::vector<BlinkPattern>;

	/// Define optional static color for each state.
	using StateColors   = std::vector<optional<Color>>;


	class LedOverride {
		std::unique_ptr<IStateDetector> 	detector;
		StatePatterns						patterns;
		unsigned							lastState = UINT_MAX;

	public:
		LedOverride(LedOverride&&) noexcept = default;
		LedOverride(const LedOverride&);

		IStateDetector&		Detector()		{ return *detector; }
		
		optional<Color>		CalcResult(const SimClient::SimvarList& nextValues);

		template<class D, class States>
		LedOverride(D detector, States patterns) :
			detector { new D { std::move(detector) } },
			patterns { ToPatterns(std::move(patterns)) }
		{
		}

		template<class States>
		LedOverride(std::unique_ptr<IStateDetector> detector, States patterns) :
			detector { std::move(detector) },
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



	// TODO: name vs. LedControl?
	class LedController {
		const uint32_t				ledId;
		const bool					isMulticolor;
		Color						defaultColor;
		Color						lastColor;
		std::vector<LedOverride>	overrides;

	public:
		LedController(UniLed, bool  defaultOn,	std::vector<LedOverride>);
		LedController(BiLed,  Color defaultCol, std::vector<LedOverride>);

		void RegisterVariables(SimClient::DedupSimvarRegister&);

		void ApplyDefault(DOHelper::X52Output&);

		void Update(DOHelper::X52Output&, const SimClient::SimvarList&);

	// Cannot read defaults
	//	void Reset(const DOHelper::X52Output&);				// TODO if viable - maybe ctor
	};


}	// namespace FSMfd::Led
