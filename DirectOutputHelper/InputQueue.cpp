#include "InputQueue.h"

#include "DirectOutputInstance.h"
#include "Utils/Debug.h"
#include "Saitek/DirectOutputImpl.h"



namespace DOHelper
{

	// helps hiding Windows.h inclusion just for DWORD...
	struct InputQueue::CallbackConverter {
		static void __stdcall OnPageChange(void* dev, DWORD dwPageId, bool activated, void* pCtxt)
		{
			uint32_t pageId = dwPageId;
			InputQueue::OnPageChange(dev, pageId, activated, pCtxt);
		}

		static void __stdcall OnButtonPress(void* dev, DWORD dwButtonId, void* pCtxt)
		{
			uint32_t buttonId = dwButtonId;
			InputQueue::OnButtonPress(dev, buttonId, pCtxt);
		}
	};



	InputQueue::InputQueue(DirectOutputInstance& source, void* device) :
		Source       { source },
		DeviceHandle { device }
	{
		Source.library->RegisterPageCallback	  (device, &CallbackConverter::OnPageChange,  this);
		Source.library->RegisterSoftButtonCallback(device, &CallbackConverter::OnButtonPress, this);
	}


	InputQueue::~InputQueue()
	{
		// Unregister
		Source.library->RegisterPageCallback(DeviceHandle, nullptr, nullptr);
		Source.library->RegisterSoftButtonCallback(DeviceHandle, nullptr, nullptr);
	}


	/*static*/ void InputQueue::OnPageChange(void* device, uint32_t pageId, bool activated, void *pCtxt)
	{
		auto& self = *reinterpret_cast<InputQueue*> (pCtxt);
		LOGIC_ASSERT (self.DeviceHandle == device);

		MessageKind kind = activated ? MessageKind::PageActivated : MessageKind::PageDeactivated;

		self.Push(kind, pageId);
	}


	/*static*/ void InputQueue::OnButtonPress(void* device, uint32_t buttonId, void *pCtxt)
	{
		auto& self = *reinterpret_cast<InputQueue*> (pCtxt);
		LOGIC_ASSERT (self.DeviceHandle == device);

		self.Push(MessageKind::ButtonPress, buttonId);
	}


	void InputQueue::Push(MessageKind kind, uint32_t data)
	{
		{
			std::unique_lock lock { mutex };

			// NOTE: time should be set here to ensure monotony inside the queue
			//		 DirectOutput provides no order for events but has own threads.
			TimePoint stamp = TimePoint::clock::now();
			messages.push_back({ stamp, kind, data });
		}
		cond.notify_one();
	}


	const InputMessage* InputQueue::PeekNext() const
	{
		std::unique_lock lock { mutex };

		return messages.empty() ? nullptr : &messages.front();
	}


	const InputMessage* InputQueue::PeekNext(const TimePoint& waitEnd) const
	{
		std::unique_lock lock { mutex };

		if (messages.empty())
			cond.wait_until(lock, waitEnd);

		return (messages.empty() || waitEnd < messages.front().time)
			? nullptr 
			: &messages.front();
	}


	optional<InputMessage>  InputQueue::PopNext(const TimePoint& waitEnd)
	{
		optional<InputMessage> res;

		std::unique_lock lock { mutex };
		if (messages.empty())
			cond.wait_until(lock, waitEnd);

		if (messages.empty() || waitEnd < messages.front().time)
			return res;

		res = messages.front();
		messages.pop_front();
		return res;
	}


	InputMessage InputQueue::PopNext()
	{
		std::unique_lock lock { mutex };

		LOGIC_ASSERT_M (!messages.empty(), "Use when consumer side is sure (Peeked) in the next message!");
		
		auto res = messages.front();
		messages.pop_front();

		return res;
	}


	void InputQueue::Clear()
	{
		std::unique_lock lock { mutex };
		messages.clear();
	}


}	// namespace DOHelper
