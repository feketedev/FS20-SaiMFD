#include "BlinkPattern.h"
#include "Utils/BasicUtils.h"
#include "Utils/Debug.h"



namespace FSMfd::Led {

	using namespace Utils;



	BlinkPattern::BlinkPattern(const std::initializer_list<Element>& elems) :
		elements { elems },
		current  { 0 },
		elapsed	 { 0 }
	{
		LOGIC_ASSERT_M (!elements.empty(), "BlinkPattern requires at least one step.");
		LOGIC_ASSERT_M (IsStatic() ||
						AllOf(elements, [](auto& el) { return el.Length > 0; }),
						"Each step must have a defined length > 0.");
	}


	BlinkPattern::BlinkPattern(optional<Color> c) : BlinkPattern { Element { c, 0 } }
	{
	}


	void BlinkPattern::Step(unsigned ticks)
	{
		while (!IsStatic() && ticks > 0)
		{
			Element& curr = elements[current];
			elapsed += ticks;
			const bool next = (curr.Length <= elapsed);
			
			ticks = next ? elapsed - curr.Length : 0;
			current += next;
			current *= current < elements.size();
			elapsed *= !next;
		}
	}


	void BlinkPattern::Reset()
	{
		current = 0;
		elapsed = 0;
	}


}	// namespace FSMfd::Led
