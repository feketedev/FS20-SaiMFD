#include "InputQueue.h"

#include "DirectOutputInstance.h"
#include "DirectOutputError.h"
#include "X52Output.h"
#include "X52Page.h"
#include "Utils/BasicUtils.h"

#include "Saitek/DirectOutputImpl.h"
#include <algorithm>
#include <iostream>
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
		DBG_ASSERT_M (pages.empty(), "Remove pages before destruction!");

		if (inputQueue != nullptr)	// move
		{
			DirectOutput().HandleReleased(Handle());
			inputQueue.reset();
		}
	}


	void*	X52Output::Handle() const noexcept
	{
		return inputQueue->deviceHandle;
	}


	DirectOutputInstance&	X52Output::DirectOutput() const noexcept
	{
		return inputQueue->source;
	}


	bool	X52Output::IsConnected() const noexcept
	{
		return DirectOutput().IsConnected(Handle());
	}

#pragma endregion



#pragma region InputEvents

	void X52Output::ProcessMessages(Duration dur)
	{
		TimePoint now = TimePoint::clock::now();
		return ProcessMessages(now + dur);
	}


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
				std::cout << " \tPage OFF " << msg.optData << std::endl;
				// TODO: Is it received during removal?
				if (activePage == nullptr)
				{
					Debug::Warning("Received duplicated page change!");
					break;
				}
				LOGIC_ASSERT (activePage != nullptr && activePage->id == msg.optData);
				activePage->OnDeactivate(msg.time);
				activePage = nullptr;
				break;

			case MessageKind::PageActivated:
			{
				std::cout << " \tPage ON  " << msg.optData << std::endl;
				LOGIC_ASSERT (activePage == nullptr || activePage->id == msg.optData);
				if (activePage != nullptr)
					break;

				auto it = std::find_if(pages.begin(), pages.end(), [&](Page* p) {
					return p->id == msg.optData;
				});
				LOGIC_ASSERT(it != pages.end());

				bool reenter = (activePage == nullptr);
				
				activePage = *it;
				if (reenter)
					RestoreLeds();
				activePage->Activate(msg.time);
				break;
			}

			case MessageKind::ButtonPress:
			{
				std::cout << " \tButton     " << msg.optData << std::endl;

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
		AddPage(p, TimePoint::clock::now());
	}


	void X52Output::AddPage(Page& pg, TimePoint causeStamp, bool activate)
	{
		LOGIC_ASSERT_M (!pg.IsAdded(), "Page already in use!");
		LOGIC_ASSERT_M (!HasPage(pg.id), "Duplicate page id!");

		DWORD flag = activate ? FLAG_SET_AS_ACTIVE : 0;
		pages.push_back(&pg);
		pg.device = this;

		SAI_ASSERT (DirectOutput().library->AddPage(Handle(), pg.id, nullptr, flag));

		// - No Activated event raised by driver using this flag!
		// - backlight gets unstable without that yield! :o
		if (activate)
		{
			activePage = &pg;
			std::this_thread::yield();
			pg.Activate(causeStamp);
		}
	}


	void X52Output::ClearPages()
	{
		auto now = TimePoint::clock::now();
		for (Page* p : pages)
			DisconnectPage(*p, now);

		pages.clear();
	}


	void X52Output::RemovePage(Page& p)
	{
		RemovePage(p, TimePoint::clock::now());
	}


	void X52Output::RemovePage(Page& p, TimePoint causeStamp)
	{
		LOGIC_ASSERT (p.CurrentDevice() == this);

		DisconnectPage(p, causeStamp);
		auto it = std::find(pages.begin(), pages.end(), &p);
		pages.erase(it);
	}


	void X52Output::DisconnectPage(Page& p, TimePoint stamp)
	{
		// won't receive it once removed from pages
		if (&p == activePage)
		{
			p.OnDeactivate(stamp);
			activePage = nullptr;	// TODO: who notifies activated page?
		}
		p.device = nullptr;

		if (IsConnected())
			SAI_ASSERT(DirectOutput().library->RemovePage(Handle(), p.id));
	}


	void X52Output::PageDestroys(Page& p) noexcept
	{
		DBG_ASSERT (p.CurrentDevice() == this);
		
		if (&p == activePage)
			activePage = nullptr;	// TODO: who notifies activated page?

		auto it = std::find(pages.begin(), pages.end(), &p);
		pages.erase(it);
		p.device = nullptr;

		if (IsConnected())
		{
			HRESULT hr = DirectOutput().library->RemovePage(Handle(), p.id);
			DBG_ASSERT (SUCCEEDED(hr));
		}
	}


	auto X52Output::FindPage(uint32_t id) const -> Page*
	{
		auto it = std::find_if(pages.begin(), pages.end(), [=](const Page* p) {
			return p->id == id;
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
			DirectOutput().library->SetLed(Handle(), activePage->id, id, on)
		);
		ledStates[id] = on;
	}


	void  X52Output::RestoreLeds()
	{
		for (uint8_t id = 0; id < ledStates.size(); id++)
		{
			if (ledStates[id].has_value())
				SetLedComponent(id, *ledStates[id]);
		}
	}

#pragma endregion


}	// namespace DOHelper

