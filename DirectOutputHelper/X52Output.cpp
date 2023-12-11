#include "InputQueue.h"

#include "DirectOutputInstance.h"
#include "DirectOutputError.h"
#include "InputMessage.h"
#include "X52Output.h"
#include "X52Page.h"
#include "Utils/BasicUtils.h"

#include "Saitek/DirectOutputImpl.h"
#include <algorithm>
#include <shared_mutex>



namespace DOHelper
{
	using Utils::AsIndex;



#pragma region BasicQuery+Special

	// needed by unique_ptr<InputQueue>
	X52Output::X52Output(X52Output&&) noexcept = default;


	X52Output::X52Output(void* const handle, DirectOutputInstance& owner) :
		inputQueue	  { new InputQueue { owner, handle } },
		wheelDebounce { SoftButton_Up, SoftButton_Down, SoftButton_Select }
	{
	}


	X52Output::~X52Output()
	{
		if (inputQueue != nullptr)		// not moved
		{
			// NOTE: A DirectOutput failure is probably not a pborlem here,
			//		 as current Handle is going to be discarded.
			for (Page* p : pages)
				OrphanPageNoEx(*p);

			DirectOutput().HandleReleased(Handle());
			inputQueue.reset();
		}
	}


	void*	X52Output::Handle() const noexcept
	{
		return inputQueue->DeviceHandle;
	}


	DirectOutputInstance&	X52Output::DirectOutput() const noexcept
	{
		return inputQueue->Source;
	}


	bool	X52Output::IsConnected() const noexcept
	{
		return DirectOutput().IsConnected(Handle());
	}

#pragma endregion



#pragma region InputEvents

	void X52Output::ProcessMessages(TimePoint until)
	{
		while (ProcessNextMessage(until));
	}


	bool X52Output::ProcessNextMessage(TimePoint waitUntil)
	{
		optional<InputMessage> msg = wheelDebounce.Receive(*inputQueue, waitUntil);
		if (msg.has_value())
			Dispatch(*msg);

		return msg.has_value();
	}


	void X52Output::Dispatch(const InputMessage& msg)
	{
		switch (msg.kind)
		{
			case MessageKind::PageDeactivated:
				Debug::Info("DirectOutput Helper", "Page OFF ", msg.optData);
				if (activePage == nullptr)
				{
					Debug::Warning("Received duplicated page Deactivation!");
					break;
				}
				else
				{
					// workaround: Deactivation after adding a new top page is already handled!
					auto it = Utils::Find(actPageBeforeAdd, msg.optData);
					if (it != actPageBeforeAdd.end())
					{
						actPageBeforeAdd.erase(it);
						break;
					}
				}
				LOGIC_ASSERT (activePage != nullptr && activePage->Id == msg.optData);
				activePage->OnDeactivate(msg.time);
				activePage = nullptr;
				break;

			case MessageKind::PageActivated:
			{
				Debug::Info("DirectOutput Helper", "Page ON  ",  msg.optData);

				// TODO: Maybe we could account for reordered Activate-Deactivate pairs too
				//		 - which still won't solve some errors I've seen...
				LOGIC_ASSERT (activePage == nullptr || activePage->Id == msg.optData);
				if (activePage != nullptr)
				{
					Debug::Warning("Received duplicated page Activation!");
					break;
				}

				activePage = FindPage(msg.optData);
				LOGIC_ASSERT(activePage != nullptr);
				
				RestoreLeds();
				activePage->Activate(msg.time);
				break;
			}

			case MessageKind::ButtonPress:
			{
				Debug::Info("DirectOutput Helper", "Button    ", msg.optData);

				LOGIC_ASSERT (activePage != nullptr);	// filtered by driver

				if (msg.optData & SoftButton_Select)
					activePage->OnButtonPress(msg.time);

				bool up = (msg.optData & SoftButton_Up);
				bool down = (msg.optData & SoftButton_Down);

				DBG_ASSERT (!up || !down);
				if (up)
					activePage->OnScroll(true, msg.time);
				if (down)
					activePage->OnScroll(false, msg.time);

				constexpr DWORD any = SoftButton_Select | SoftButton_Up | SoftButton_Down;
				DBG_ASSERT_M (0 == (msg.optData & ~any), "Unknown keypress");
				break;
			}

			default:
				DBG_BREAK;
		}
	}

#pragma endregion



#pragma region Pages

	bool X52Output::HasPage(uint32_t id) const
	{
		return FindPage(id) != nullptr;
	}


	bool X52Output::HasPages() const
	{
		return pages.empty();
	}


	void X52Output::AddPage(Page& p, bool activate)
	{
		AddPage(p, TimePoint::clock::now(), activate);
	}


	void X52Output::AddPage(Page& pg, TimePoint causeStamp, bool activate)
	{
		LOGIC_ASSERT_M (!pg.IsAdded(), "Page already in use!");
		LOGIC_ASSERT_M (!HasPage(pg.Id), "Duplicate page id!");

		DWORD flag = activate ? FLAG_SET_AS_ACTIVE : 0;
		pages.push_back(&pg);
		pg.device = this;

		SAI_ASSERT (DirectOutput().library->AddPage(Handle(), pg.Id, nullptr, flag));

		// - No Activated event raised by driver using this flag!
		// - Deactivated is received though later...
		// - backlight gets unstable without that yield! :o
		if (activate)
		{
			if (activePage != nullptr)
			{
				actPageBeforeAdd.push_back(activePage->Id);
				activePage->OnDeactivate(causeStamp);
			}
			activePage = &pg;
			std::this_thread::yield();
			RestoreLeds();
			pg.Activate(causeStamp);
		}
	}


	void X52Output::ClearPages()
	{
		// won't receive Deactivate from DirectOutput
		if (activePage != nullptr)
			activePage->OnDeactivate(TimePoint::clock::now());
		activePage = nullptr;

		for (Page* p : pages)
			OrphanPage(*p);

		pages.clear();
	}


	void X52Output::RemovePage(Page& p)
	{
		RemovePage(p, TimePoint::clock::now());
	}


	void X52Output::RemovePage(Page& p, TimePoint causeStamp)
	{
		LOGIC_ASSERT (p.CurrentDevice() == this);

		const bool wasActive = &p == activePage;
		
		// won't receive Deactivate from DirectOutput
		if (wasActive)
			p.OnDeactivate(causeStamp);

		LOGIC_ASSERT (BookRemovedPage(p));
		OrphanPage(p);

		DBG_ASSERT (!wasActive || activePage == nullptr);
	}


	void X52Output::OrphanPage(Page& p) const
	{
		p.device = nullptr;
		if (IsConnected())
			SAI_ASSERT(DirectOutput().library->RemovePage(Handle(), p.Id));
	}


	// NOTE: In case of FAILED(hr), state of our objects still remain defined,
	//		 although the state of hardware is unsure.
	void X52Output::OrphanPageNoEx(Page& p) const noexcept
	{
		DBG_ASSERT (p.device == this);

		if (p.device == this && inputQueue != nullptr && IsConnected())
		{
			HRESULT hr = DirectOutput().library->RemovePage(Handle(), p.Id);
			if (FAILED(hr))
				Debug::Warning("DirectOutput Helper", "Failed to delete page from x52 device.");
		}
		p.device = nullptr;
	}


	/// Deletes @p p from @a pages and elects next activePage (following DirectOutput).
	/// @returns Found and booked (success)
	bool X52Output::BookRemovedPage(const Page& p) noexcept
	{
		const auto it = Utils::Find(pages, &p);
		if (it == pages.end())
			return false;

		// Seems that DirectOutput just throws to built-in profile page.
		// - at least an Activate won't be necessary...
		// Also: Deleting a background Page seems to be unreliable :(
		if (activePage == &p)
			activePage = nullptr;

		pages.erase(it);
		return true;
	}


	void X52Output::PageDestroys(Page& p) noexcept
	{
		DBG_ASSERT (p.CurrentDevice() == this);
		
		const bool wasActive = &p == activePage;

		bool found = BookRemovedPage(p);
		DBG_ASSERT_M (found, "Hard to beleive internal error.");
		DBG_ASSERT (!wasActive || activePage == nullptr);

		// NOTE: A swallowed failure here would meen the Page might not have
		//		 been deleted from the X52 device. Fully correct solution
		//		 could be flagging erroneous state to throw on next public call
		//		 which could lead to Reset the library.
		//		 As long as this is just game equipment, waiting to encounter
		//		 the next SAI_ASSERT seems good enough.
		OrphanPageNoEx(p);
	}


	auto X52Output::FindPage(uint32_t id) const noexcept -> Page*
	{
		auto it = Utils::FindIf(pages, [=](const Page* p) {
			return p->Id == id;
		});

		return it == pages.end() ? nullptr : *it;
	}

#pragma endregion



#pragma region LEDs

	void X52Output::SetColor(BiLed id, LedColor color)
	{
		uint8_t trg = static_cast<uint8_t>(color);
		if (activePage)
		{
			SetLedComponent(id,		trg & 2);
			SetLedComponent(id + 1, trg & 1);
		}
	}


	void X52Output::SetState(UniLed id, bool on)
	{
		if (activePage)
			SetLedComponent(id, on);
	}


	void  X52Output::SetLedComponent(uint8_t id, bool on)
	{
		if (ledStates[id] == on)
			return;

		SAI_ASSERT (
			DirectOutput().library->SetLed(Handle(), activePage->Id, id, on)
		);
		ledStates[id] = on;
	}


	void  X52Output::RestoreLeds()
	{
		const DWORD pgId = activePage->Id;

		for (uint8_t id = 0; id < ledStates.size(); id++)
		{
			if (ledStates[id].has_value())
			{
				bool on = *ledStates[id];
				SAI_ASSERT (
					DirectOutput().library->SetLed(Handle(), pgId, id, on)
				);
			}
		}
	}

#pragma endregion


}	// namespace DOHelper

