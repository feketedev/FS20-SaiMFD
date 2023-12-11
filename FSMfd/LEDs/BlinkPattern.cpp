#include "BlinkPattern.h"
#include "Utils/BasicUtils.h"
#include "Utils/Debug.h"



namespace FSMfd::Led
{
	using namespace Utils;



	BlinkPattern::BlinkPattern(const std::initializer_list<Element>& elems) :
		elements { elems },
		current  { 0 },
		elapsed	 { 0 }
	{
		LOGIC_ASSERT_M (!elements.empty(), "BlinkPattern requires at least one step.");
		LOGIC_ASSERT_M (AllOf(elements, [](auto& el) { return el.Length > Duration::zero(); }),
						"Each step must have a defined length > 0.");
	}


	BlinkPattern::BlinkPattern(optional<Color> c) :
		BlinkPattern { Element { c, Duration::max() }}
	{
	}


	void BlinkPattern::Advance(Duration dur)
	{
		while (!IsStatic() && dur.count() > 0)
		{
			Element& curr = elements[current];
			elapsed += dur;
			const bool next = (curr.Length <= elapsed);
			
			dur = next ? elapsed - curr.Length : Duration::zero();

			current += next;
			current *= current < elements.size();
			elapsed *= !next;
		}
	}


	void BlinkPattern::Reset()
	{
		current = 0;
		elapsed = Duration::zero();
	}


}	// namespace FSMfd::Led
