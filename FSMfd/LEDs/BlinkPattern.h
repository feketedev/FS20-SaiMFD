#pragma once

#include "FSMfdTypes.h"
#include <vector>



namespace FSMfd::Led {

	/// Generalized blinking effect or static light.
	class BlinkPattern {
	public:
		struct Element {
			const optional<Color>	Color;
			const unsigned			Length;
		};

	private:
		std::vector<Element>	elements;
		unsigned				current;
		unsigned				elapsed;

	public:
		/// Define a series of optional override Colors, each 
		/// associated with a timespan defined in update ticks.
		BlinkPattern(const std::initializer_list<Element>&);

		/// Define a static color.
		BlinkPattern(optional<Color>);

		bool				IsStatic()	   const	{ return elements.size() == 1; }
		unsigned			StepCount()	   const	{ return elements.size(); }
		unsigned			CurrentStep()  const	{ return current; }
		optional<Color>		CurrentColor() const	{ return elements[current].Color; }

		void	Step(unsigned ticks);
		void	Reset();
	};


}	// namespace FSMfd::Led