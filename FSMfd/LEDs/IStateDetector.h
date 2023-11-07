#pragma once

#include "FSMfdTypes.h"
#include <memory>



namespace FSMfd::Led {

	class IStateDetector {
	public:
		virtual void				RegisterVariables(SimClient::DedupSimvarRegister&) = 0;
		virtual unsigned			StateCount()								 const = 0;
		virtual optional<unsigned>	DeterminState(const SimClient::SimvarList&)	 const = 0;

		virtual std::unique_ptr<IStateDetector> Clone() const = 0;
		virtual ~IStateDetector();
	};


}	// namespace FSMfd::Led
