#pragma once

#include "InputMessage.h"
#include <mutex>
#include <deque>
#include <optional>



namespace DOHelper
{

	/// Synchronized buffer for input events of 1 device, to be processed by the thread handling outputs.
	class InputQueue {
	public:
		DirectOutputInstance&				Source;
		void* const		 					DeviceHandle;

	private:
		mutable std::mutex					mutex;
		mutable std::condition_variable 	cond;
		std::deque<InputMessage>			messages;


		static void OnPageChange (void* device, uint32_t pageId, bool activated, void* pCtxt);
		static void OnButtonPress(void* device, uint32_t buttonId,				 void* pCtxt);

		void Push(MessageKind kind, uint32_t data);

	public:
		const InputMessage* 		PeekNext()							const;
		const InputMessage* 		PeekNext(const TimePoint& waitEnd)	const;
		optional<InputMessage>		PopNext(const TimePoint& waitEnd);
		InputMessage				PopNext();
		void						Clear();

		InputQueue(DirectOutputInstance& source, void* deviceHandle);
		~InputQueue();

		InputQueue(InputQueue&&) = delete;		// pinned address for DirectOutput callbacks

		struct CallbackConverter;				// internal
	};


}	// namespace DOHelper
