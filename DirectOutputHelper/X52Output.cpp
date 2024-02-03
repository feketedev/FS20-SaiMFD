#include "InputQueue.h"

#include "DirectOutputInstance.h"
#include "DirectOutputError.h"
#include "InputMessage.h"
#include "X52Output.h"
#include "X52Page.h"
#include "Utils/BasicUtils.h"
#include "Utils/CastUtils.h"

#include "Saitek/DirectOutputImpl.h"
#include <algorithm>
#include <shared_mutex>



namespace DOHelper
{
	using Utils::Cast::AsIndex;

	constexpr const char* LogSource = "DirectOutput Helper";



#pragma region BasicQuery+Special

	// needed by unique_ptr<InputQueue>
	X52Output::X52Output(X52Output&&) noexcept = default;


	X52Output::X52Output(void* handle, DirectOutputInstance& owner) :
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
		{
			Dispatch(*msg);
			return true;
		}
		
		// Rare but existing case: No PageActivated received at all --> need to trigger activePage recovery!
		//   Timing of this recovery is rather arbitrary - idle timeout of input message processing seems appropriate.
		//   A further late PageActivated message (missed bc of recovery) should be handled gracefully.
		if (activePageLagsBehind && inputQueue->PeekNext() == nullptr)
			RecoverActivePage(waitUntil);

		return false;
	}


	void X52Output::Dispatch(const InputMessage& msg)
	{
		switch (msg.kind)
		{
			case MessageKind::PageDeactivated:
			{
				Debug::Info("DirectOutput", "\tDeactivatePage\t", msg.optData);

				// Just a warning-avoidance workaround for reordered events. (Activate messages drive paging.)
				if (msg.optData == expectedPageDeactivation)
				{
					expectedPageDeactivation.reset();
					break;
				}

				if (activePage == nullptr || activePage->Id != msg.optData)
				{
					Debug::Warning(LogSource, "Duplicated or invalid page Deactivation! Ignoring.");
					break;
				}

				DeactivateCurrentPage(msg.time);
				activePageLagsBehind = false;
				break;
			}

			// Trying to tolerate reordered or unbalanced Activate-Deactivate messages
			// -> Activation messages drive our state, Deactivation is secondary
			// -> still can get out-of-sync with DirectOutput (due to missed messages),
			//    aim is to be able to catch up at repeated scroll inputs
			case MessageKind::PageActivated:
			{
				Debug::Info("DirectOutput", "\tActivatePage\t", msg.optData);

				if (activePage != nullptr && activePage->Id == msg.optData)
				{
					Debug::Warning("Received duplicated page Activation! Ignoring.");
					break;
				}

				if (Page* deact = DeactivateCurrentPage(msg.time))
					expectedPageDeactivation = deact->Id;
				
				TryActivatePage(FindPage(msg.optData), msg.time);
				break;
			}

			case MessageKind::ButtonPress:
			{
				Debug::Info(LogSource, "Button    ", msg.optData);

				if (activePage == nullptr)
				{
					// Not even just lagging behind DirectOutput - state got messed up!
					Debug::Warning(LogSource, "Keypress with no active Page.");
					DBG_BREAK;
					inputQueue->Clear();
					RecoverActivePage(msg.time);
					break;
				}

				if (msg.optData & SoftButton_Select)
					activePage->OnButtonPress(msg.time);

				bool up   = (msg.optData & SoftButton_Up);
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

	// To avoid interpreting pending inputs addressed to an already removed Page.
	// NOTE 1: No guarantee on theoretic level, as we can't synch with DirectOutput's internal state. Yield can help in practice.
	// NOTE 2: Locking in InputQueue is not noexcept, but should throw only in case of program error, in which case we abort.
	static void ClearForPageRemoved(InputQueue& queue) noexcept
	{
		std::this_thread::yield();
		queue.Clear();
	}


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

		// - No Activated event raised by driver using this flag -> handle page change in place!
		// - Deactivated gets posted though asynchronously, provided that any page was active
		if (activate)
		{
			DeactivateCurrentPage(causeStamp);
			TryActivatePage(&pg, causeStamp);
			
			// NOTE: We have no perfect means to sync with the driver (e.g. if a user page-turn is just underway).
			// Clearing unserved events (+ the potentially posted Deactivated message) is the simplest solution in practice.
			inputQueue->Clear();
			expectedPageDeactivation.reset();
		}
	}


	void X52Output::ClearPages()
	{
		// won't receive Deactivate from DirectOutput
		DeactivateCurrentPage(TimePoint::clock::now());

		for (Page* p : pages)
			OrphanPage(*p);

		pages.clear();
		ClearForPageRemoved(*inputQueue);
		expectedPageDeactivation.reset();
	}


	void X52Output::RemovePage(Page& p, bool activateNeighbor)
	{
		RemovePage(p, TimePoint::clock::now(), activateNeighbor);
	}


	void X52Output::RemovePage(Page& p, TimePoint causeStamp, bool activateNeighbor)
	{
		LOGIC_ASSERT (p.CurrentDevice() == this);

		const bool wasActive = &p == activePage;
		Page*      next      = nullptr;
		if (wasActive && activateNeighbor && pages.size() > 1)
		{
			auto it = Utils::Find(pages, &p);
			next = (it == pages.begin()) ? *(it + 1) : *(it - 1);
		}

		
		// won't receive Deactivate from DirectOutput
		if (wasActive)
			DeactivateCurrentPage(causeStamp);

		OrphanPage(p);
		LOGIC_ASSERT (BookRemovedPage(p));
		DBG_ASSERT (!wasActive || activePage == nullptr);

		if (next != nullptr)
		{
			// nice li'l hack to set active page manually
			SAI_ASSERT (DirectOutput().library->RemovePage(Handle(), next->Id));
			SAI_ASSERT (DirectOutput().library->AddPage(Handle(), next->Id, nullptr, FLAG_SET_AS_ACTIVE));
			TryActivatePage(next, causeStamp);
		}
	}

	
	void X52Output::PageDestroys(Page& p) noexcept
	{
		DBG_ASSERT (p.CurrentDevice() == this);
		
		const bool wasActive = &p == activePage;

		// NOTE: A swallowed failure here would mean the Page might not have
		//		 been deleted from the X52 device. Fully correct solution
		//		 could be flagging erroneous state to throw on next public call
		//		 which could lead to Reset the library.
		//		 As long as this is just game equipment, waiting to encounter
		//		 the next SAI_ASSERT seems good enough.
		OrphanPageNoEx(p);

		bool found = BookRemovedPage(p);
		DBG_ASSERT_M (found, "Hard-to-beleive internal error.");
		DBG_ASSERT (!wasActive || activePage == nullptr);
	}


	void X52Output::OrphanPage(Page& p) const
	{
		p.device = nullptr;
		if (IsConnected())
			SAI_ASSERT (DirectOutput().library->RemovePage(Handle(), p.Id));
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
				Debug::Warning(LogSource, "Failed to delete page from x52 device.");
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
		if (activePage == &p)
		{
			Debug::Info(LogSource, "Page OFF without OnDeactivate [noexcept mode]", p.Id);
			activePage = nullptr;
			expectedPageDeactivation.reset();						// won't be received
		}

		pages.erase(it);
		ClearForPageRemoved(*inputQueue);
		return true;
	}

	
	auto X52Output::DeactivateCurrentPage(TimePoint stamp) -> Page*
	{
		if (activePage != nullptr)
		{
			Utils::OnExitAssignment revertAnyway { activePage, nullptr };

			Debug::Info(LogSource, "Page OFF\t", activePage->Id);
			activePage->OnDeactivate(stamp);
			return activePage;
		}
		return nullptr;
	}


	auto X52Output::FindPage(uint32_t id) const noexcept -> Page*
	{
		auto it = Utils::FindIf(pages, [=](const Page* p) {
			return p->Id == id;
		});

		return it == pages.end() ? nullptr : *it;
	}



#pragma region Recovery

	/// Run any DirectOutput command that requires current page ID.
	template<class SaiActionPtr, class... Args>
	bool X52Output::TryWithActivePage(const SaiActionPtr& act, const Args&... args)
	{
		DBG_ASSERT (activePage != nullptr);

		if (activePageLagsBehind)
			return false;

		Saitek::DirectOutput& lib = *DirectOutput().library;
		DWORD   id = activePage->Id;
		HRESULT hr = (lib.*act)(Handle(), id, args...);

		if (SUCCEEDED(hr))
			return true;

		if (hr == Page::NotActiveError)
		{
			activePageLagsBehind = true;
			return false;
		}

		throw DirectOutputError { hr, "Operation failed with unknown error." };
	}


	/// Virtually transactional, except the extra OnActivate-OnDeactivate pair getting called on failure.
	/// @returns false              if @p page is not currently active according to DirectOutput
	/// @throws DirectOutputError:  on unexpected errors during page activation
	/// @throws unkown exeptions:   encountered during OnActivate/OnDeactivate
	bool X52Output::TryActivatePage(Page* page, TimePoint stamp)
	{
		LOGIC_ASSERT (activePage == nullptr && page != nullptr);
		
		Debug::Info(LogSource, "Page ON \t", page->Id);

		bool activateCalled = false;
		try
		{
			activePage = page;
			RestoreLeds();
			activateCalled = true;
			activePage->Activate(stamp);
			if (expectedPageDeactivation == page->Id)
				expectedPageDeactivation.reset();

			activePageLagsBehind = false;
			return true;
		}
		catch (const DirectOutputError& err)
		{
			Utils::OnExitAssignment revertAnyway { activePage, nullptr };
			activePageLagsBehind = true;

			Debug::Warning(LogSource, "Failed to activate page. Error:", err.ErrorCode);

			// driver state ignored - ours restored
			if (activateCalled)
				activePage->OnDeactivate(stamp);	// may throw on its own

			if (err.ErrorCode == Page::NotActiveError)
				return false;
			
			throw;									// unexpected D.O. error
		}
		catch (...)
		{
			activePage = nullptr;
			activePageLagsBehind = true;
			
			Debug::Warning(LogSource, "Failed to activate page due to external error.");
			throw;
		}
	}


	bool X52Output::RecoverActivePage(TimePoint stamp)
	{
		DBG_ASSERT (activePageLagsBehind);

		Debug::Warning(LogSource, "Attempting to recover active page...   ");
		
		Page* const found = FindActivePageByProbing();

		if (found == activePage)
		{
			// probably missed a back-and-forth paging
			inputQueue->Clear();
			expectedPageDeactivation.reset();
			Debug::Warning(LogSource, "Current is correct!");
			activePageLagsBehind = false;
			return true;
		}

		DeactivateCurrentPage(stamp);
		if (found == nullptr)
		{
			Debug::Warning(LogSource, "Neither is active.");
			activePageLagsBehind = false;
			return true;
		}

		Debug::Warning(LogSource, "Found:", found->Id);
		if (TryActivatePage(found, stamp))
		{
			inputQueue->Clear();
			expectedPageDeactivation.reset();
			return true;
		}
		return false;
	}


	auto X52Output::FindActivePageByProbing() const -> Page*
	{
		const size_t count = pages.size();

		size_t origo = count;
		if (activePage != nullptr)
		{
			origo = 0;
			while (origo < count && pages[origo]->Id != activePage->Id)
				++origo;

			DBG_ASSERT (origo < count);
		}

		for (size_t i = 0; i + 1 < count; i++)
		{
			size_t dist = i / 2 + 1;
			int		dir = (i % 2) ? -1 : +1;

			Page* pg = pages[(origo + dir * dist + count) % count];
			if (pg->ProbeActive())
				return pg;
		}
		return nullptr;
	}


	bool X52Output::TrySetActivePageLine(uint32_t i, uint32_t len, const wchar_t* text)
	{
		return TryWithActivePage(&Saitek::DirectOutput::SetString, i, len, text);
	}

#pragma endregion

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
		bool doSwitch = (ledStates[id] != on) && (activePage != nullptr);
		ledStates[id] = on;
		if (doSwitch)
			TryWithActivePage(&Saitek::DirectOutput::SetLed, id, on);
	}


	void  X52Output::RestoreLeds()
	{
		DBG_ASSERT (activePage != nullptr);

		bool actUnchanged = true;
		for (uint8_t id = 0; id < ledStates.size() && actUnchanged; id++)
		{
			if (ledStates[id].has_value())
			{
				bool      on = *ledStates[id];
				actUnchanged = TryWithActivePage(&Saitek::DirectOutput::SetLed, id, on);
			}
		}
	}

#pragma endregion


}	// namespace DOHelper

