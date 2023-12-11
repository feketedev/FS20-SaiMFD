#pragma once

#include "DOHelperTypes.h"
#include <cstdint>


namespace DOHelper
{	
	using namespace std::chrono_literals;

	class  InputQueue;
	struct InputMessage;



	/// A consumer-side filter to get cleaner inputs from X52's scrolling wheel.
	class ScrollwheelDebounce {

		static constexpr Duration PressCancelDelay	 = 40ms;
		static constexpr Duration OppositePressDelay = 40ms;
		static constexpr Duration OppositeDelayLimit = 120ms;
		static constexpr Duration ScrollPressCancel  = 400ms;


		// Outside last: start timer + send Event
		// Next within timer: just ignore (no long press yet)

		// 0 --> emittable --> cancelling --> 0
		//    `press1       `emit          `timeout
		struct PressTracker {
			const uint32_t	bitmask;
			TimePoint		last;
			bool			emittable = false;
			
			void		Receive(uint32_t presses, const TimePoint& at);
			void		Reset();
			uint32_t	Emit();
		};


		// 0 ---> emitPend (counting) ----> cancelling ----> 0
		//    `press1                 |                 `timeout
		//                            `timeout/threshold/event => emit
		struct ScrollTracker {
			const uint32_t	upMask;
			const uint32_t	downMask;
			TimePoint		first;
			TimePoint		last;
			bool			emitPend = false;
			short			balance  = 0;

			bool IsEmittableAt(TimePoint at) const;
			bool IsCancellingAt(TimePoint at) const;
			void Receive(uint32_t presses, const TimePoint& at);

			/// Resolve accumulated presses if emitPend
			/// @returns Debounced scroll input - Up, Down or 0 (!)
			uint32_t Emit();
		};


		// could be more if DirectInput could utilize the second scroll wheel...
		PressTracker	press;			// ~ slave role, can be delayed by scroll
		ScrollTracker	scroll;			// ~ master role

		bool			lastScrollWasPressed = false;

		const uint32_t	relevantMask;

		bool		HasDataToForward(const InputMessage&) const;
		bool		HasRelevantData(const InputMessage&) const;

		uint32_t	ConfirmScroll(InputQueue&);
		bool		ProcessBounce(const InputMessage&);

	public:
		ScrollwheelDebounce(uint32_t upMask, uint32_t downMask, uint32_t pressMask);

		optional<InputMessage> Receive(InputQueue&, TimePoint until);
	};


}	// namespace DOHelper