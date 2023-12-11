#pragma once

#include "FSMfdTypes.h"
#include <memory>



namespace FSMfd::Led
{

	/// Some simple algorithm that maps certain states of Simulation Variables to numbered states.
	/// @remarks	
	///		The result in turn can trigger certain LED effects.
	///		If results Nothing: no effect will be applied (a calling LedOverride stays inactive).
	class IStateDetector {
	public:
		virtual void				RegisterVariables(SimClient::DedupSimvarRegister&) = 0;
		virtual unsigned			StateCount()								 const = 0;
		virtual optional<unsigned>	DetectState(const SimClient::SimvarList&)	 const = 0;

		virtual std::unique_ptr<IStateDetector> Clone() const = 0;
		virtual ~IStateDetector();
	};


}	// namespace FSMfd::Led
