#include "ScrollwheelDebounce.h"

#include "InputQueue.h"
#include "Utils/Debug.h"



namespace DOHelper {


	// TODO Cleanup, check Press & Scroll!

#pragma region Trackers

	void ScrollwheelDebounce::PressTracker::Receive(uint32_t presses, const TimePoint& at)
	{
		if ((presses & bitmask) == 0)
			return;

		bool newPress = last + PressCancelDelay <= at;
		last = at;
		emittable |= newPress;
	}


	void ScrollwheelDebounce::PressTracker::Reset()
	{
		last = {};
		emittable = false;
	}


	uint32_t ScrollwheelDebounce::PressTracker::Emit()
	{
		DBG_ASSERT (emittable);
		emittable = false;
		return bitmask;
	}


	bool ScrollwheelDebounce::ScrollTracker::IsEmittableAt(TimePoint at) const
	{
		return emitPend	&&
			(  balance != 0 && last + OppositePressDelay <= at
			|| balance < -2	|| 2 < balance
			|| first + OppositeDelayLimit <= at				  );
	}


	bool ScrollwheelDebounce::ScrollTracker::IsCancellingAt(TimePoint at) const
	{
		return !emitPend 
			&& at < last + OppositePressDelay
			&& at < first + OppositeDelayLimit;
	}


	void ScrollwheelDebounce::ScrollTracker::Receive(uint32_t presses, const TimePoint& at)
	{
		const bool up = 0 < (presses & upMask);
		const bool down = 0 < (presses & downMask);
		if (!up && !down)
			return;

		DBG_ASSERT_M (!IsEmittableAt(at), "Missed Emit!");

		const bool inDelayWindow = at < last  + OppositePressDelay;
		const bool inDelayLimit  = at < first + OppositeDelayLimit;
		const bool newPress		 = !inDelayWindow || !inDelayLimit;
		
		last = at;
		if (newPress)
		{
			first = at;
			balance  = up - down;
			emitPend = true;
			return;
		}
		balance += up - down;
	}


	uint32_t ScrollwheelDebounce::ScrollTracker::Emit()
	{
		DBG_ASSERT (emitPend);

		emitPend = false;

		return (0 < balance) ? upMask
			 : (0 > balance) ? downMask
			 : 0;
	}

#pragma endregion




#pragma region Combine Presses


	ScrollwheelDebounce::ScrollwheelDebounce(uint32_t upMask, uint32_t downMask, uint32_t pressMask) :
		press		 { pressMask },
		scroll		 { upMask, downMask },
		relevantMask { upMask | downMask | pressMask }
	{
	}


	bool ScrollwheelDebounce::HasDataToForward(const InputMessage& msg) const
	{
		return msg.kind != MessageKind::ButtonPress
			|| msg.optData & ~relevantMask;
	}


	bool ScrollwheelDebounce::HasRelevantData(const InputMessage& msg) const
	{
		return msg.kind == MessageKind::ButtonPress
			&& msg.optData & relevantMask;
	}



	optional<InputMessage>	ScrollwheelDebounce::Receive(InputQueue& queue, TimePoint until)
	{
		//DBG_ASSERT (!press.set && scroll.balance == 0);		// prior inputs have been handled!
		// TODO precise flags

		//if (scroll.emitPend)
		//{
		//	uint32_t debounced = ConfirmScroll(queue);
		//	if (debounced != 0)
		//		return AssembleScrollMessage(debounced);
		//}

		DBG_ASSERT (!scroll.emitPend && !press.emittable);


		while (optional<InputMessage> msg = queue.PopNext(until))
		{
			TimePoint t = msg->time;

			// NOTE: triggering emission by Peeking ahead for BoundingMessage
			//		 is done only within DebounceChain
			if (HasDataToForward(*msg))
			{
				DBG_ASSERT(!HasRelevantData(*msg));
				return *msg;
			}

			// scroll takes precedens, can delay presses -> "Press & Scroll" inputs
			scroll.Receive(msg->optData, t);
			if (scroll.emitPend)
			{
				// restart listening for during scroll
				press.Reset();
				press.Receive(msg->optData, t);

				// from here must finish regardless of TimePoint until
				uint32_t debounced = ConfirmScroll(queue);
				
				// Intedermined presses are Sinked, just like empty messages.
				if (debounced == 0)
					continue;

				// Press & scroll
				if (press.emittable)
				{
					debounced |= press.Emit();
					lastScrollWasPressed = true;
				}

				msg->optData = debounced;
				return *msg;
			}

			press.Receive(msg->optData, t);

			if (lastScrollWasPressed && t < scroll.last + ScrollPressCancel)
				continue;

			// but a single press won't be delayed to wait for a potential Press & Scroll
			if (press.emittable)
			{
				msg->optData = press.Emit();
				return *msg;
			}
		}
		return Nothing;
	}

	// TODO: whats this?
	//InputMessage ScrollwheelDebounce::AssembleScrollMessage(uint32_t debounced)
	//{
	//	// Press & scroll
	//	if (scroll.first <= press.last && press.last <= scroll.last)
	//		debounced |= press.Emit();

	//	return InputMessage {
	//		scroll.first,
	//		MessageKind::ButtonPress,
	//		debounced
	//	};
	//}


	/// @returns	Ready to emit.
	bool ScrollwheelDebounce::ProcessBounce(const InputMessage& msg)
	{
		scroll.Receive(msg.optData, msg.time);
		press.Receive(msg.optData, msg.time);

		return scroll.IsEmittableAt(msg.time)
			|| !scroll.emitPend && press.emittable;
	}


	/// Determine the single scroll direction to emit for a newly emerging chain of bounces.
	/// @returns	Up, Down or empty mask based on bounces' balance within timeout window.
	uint32_t	ScrollwheelDebounce::ConfirmScroll(InputQueue& queue)
	{
		DBG_ASSERT (scroll.emitPend);

		const TimePoint delayMax = scroll.first + OppositeDelayLimit;

		// End of continuous bouncy sequence, trigger emit
		auto isBoundary = [this](const InputMessage& n)
		{
			return HasDataToForward(n) || scroll.IsEmittableAt(n.time);
		};

		DBG_ASSERT(!scroll.IsEmittableAt(scroll.last));
		bool emittable = false;
		while (!emittable)
		{
			TimePoint windowEnd = scroll.last + OppositePressDelay;

			DBG_ASSERT (scroll.IsEmittableAt(windowEnd) && scroll.IsEmittableAt(delayMax));

			// NOTE: If next is within OppositePressDelay but over OppositeDelayLimit
			//		 [or balance > 2], it will init the Balance of a new scroll input.
			const InputMessage* next = queue.PeekNext(std::min(windowEnd, delayMax));
			
			emittable = (next == nullptr) 
						|| isBoundary(*next)
						|| ProcessBounce(queue.PopNext());
		}

		return scroll.Emit();
	}


#pragma endregion


}	// namespace DOHelper
