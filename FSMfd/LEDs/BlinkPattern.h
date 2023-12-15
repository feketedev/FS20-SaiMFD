#pragma once

#include "FSMfdTypes.h"
#include <vector>



namespace FSMfd::Led
{

	/// Generalized blinking effect or static light.
	class BlinkPattern {
	public:
		struct Element {
			const optional<Color>	Color;
			const Duration			Length;
		};

	private:
		std::vector<Element>	elements;
		unsigned				current;
		Duration				elapsed;

	public:
		/// Define a series of optional override Colors, each 
		/// associated with a timespan defined in update ticks.
		BlinkPattern(const std::initializer_list<Element>&);

		/// Define a static color.
		BlinkPattern(optional<Color>);

		bool				IsStatic()	   const	{ return StepCount() == 1; }
		unsigned			StepCount()	   const	{ return Practically<unsigned>(elements.size()); }
		unsigned			CurrentStep()  const	{ return current; }
		optional<Color>		CurrentColor() const	{ return elements[current].Color; }
		Duration			HoldTime()	   const	{ return elements[current].Length - elapsed; }

		void	Advance(Duration);
		void	Reset();
	};


}	// namespace FSMfd::Led
