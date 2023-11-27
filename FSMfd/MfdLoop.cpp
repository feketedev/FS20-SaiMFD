#include "MfdLoop.h"

#include "Configurator.h"
#include "SimClient/FSClient.h"
#include "SimClient/SimConnectError.h"
#include "Pages/Concrete/WaitSpinner.h"
#include "Pages/FSPageList.h"			//TODO
#include "LEDs/LedControl.h"

#include "DirectOutputHelper/X52Output.h"
#include "Utils/Debug.h"
#include "Utils/IoUtils.h"
#include <thread>
#include <iostream>



namespace FSMfd
{
	using namespace DOHelper;
	using namespace SimClient;
	using Pages::FSPageList;
	using Pages::SimPage;
	using Led::LedControl;


	
#pragma region Tick-Counting

	static unsigned AdvanceToUpcomingTick(TimePoint& tick, Duration ival, TimePoint now) noexcept
	{
		unsigned ticksPassed = (now < tick)
			? 0
			: 1 + (now - tick) / ival;

		tick += ival * ticksPassed;
		return ticksPassed;
	}


	static unsigned AdvanceToLaggingTick(TimePoint& tick, Duration ival, TimePoint now) noexcept
	{
		DBG_ASSERT (tick <= now);

		unsigned ticksPassed = (now - tick) / ival;

		tick += ival * ticksPassed;
		return ticksPassed;
	}


	// Keeps 6 Hz or other non-representable refresh rates in sync with others.
	class SynchClock {
		// chose method -> stay simple when period is divisible
		unsigned (SynchClock::* const advance) (TimePoint);
		
		TimePoint		lastReference;
		TimePoint		next;

	public:
		const Duration	Period;
		const Duration	Interval;
		const Duration	Remainder;

		const bool		 IsDue(TimePoint now) const 	{ return next <= now; }
		const TimePoint& Tick()				  const 	{ return next;   }
		operator const TimePoint& ()		  const 	{ return Tick(); }


		SynchClock(Duration peri, unsigned freq, TimePoint tick0) :
			Period    { peri },
			Interval  { peri / freq },
			Remainder { peri - freq * Interval },
			advance   { Remainder == Duration::zero()
						? &SynchClock::SimpleAdvance
						: &SynchClock::AdvanceInSync  }
		{
			Reset(tick0);
		}


		/// Step to wait for next tick to happen after @p now.
		/// @returns	Number of ticks elapsed since last call.
		unsigned Advance(TimePoint now) noexcept
		{
			return (this->*advance)(now);
		}


		void	 Reset(TimePoint tick0, bool doneTillNext = false) noexcept
		{
			lastReference = tick0 - Remainder - Interval;
			next          = tick0 + doneTillNext * Interval;
		}

	private:
		unsigned SimpleAdvance(TimePoint now) noexcept
		{
			return AdvanceToUpcomingTick(next, Interval, now);
		}


		unsigned AdvanceInSync(TimePoint now) noexcept
		{
			unsigned ticks     = AdvanceToUpcomingTick(next, Interval, now);
			unsigned periSteps = AdvanceToLaggingTick(lastReference, Period, now);
			next += periSteps * Remainder;

			// NOTE: not realistic to accumulate an Interval from Remainder differences...
			return ticks;
		}
	};

#pragma endregion



#pragma region LoadNotification

	bool MfdLoop::CanUse(X52Output& device) const
	{
		return uninterrupted && device.IsConnected();
	}


	bool MfdLoop::CanReload(FSClient& client) const
	{
		return uninterrupted && client.IsConnected();
	}


	bool MfdLoop::CanFlyAircraft(FSClient& client) const
	{
		return CanReload(client) && inFlight && !aircraftChanged;
	}


	void MfdLoop::Notify(NotificationCode code, uint32_t parameter, TimePoint stamp)
	{
		LOGIC_ASSERT (code == codeInFlight);

		inFlight = static_cast<bool>(parameter);
		Debug::Info(inFlight ? "Flight started." : "Flight ended.");
	}


	void MfdLoop::Notify(NotificationCode code, const char* path, TimePoint)
	{
		LOGIC_ASSERT (code == codeAircraftLoaded);
		
		aircraftChanged = true;
		Debug::Info("Aircraft changed.");
	}


	// Returns when inFlight OR connection lost with FS
	void MfdLoop::WaitForFlight(X52Output& device, FSClient& client, const Configurator& config)
	{
		Pages::WaitSpinner loadingPage { L"FS20-SaiMFD", 0, L"->>-" };
		device.AddPage(loadingPage);

		TimePoint time = TimePoint::clock::now();
		do
		{
			loadingPage.SetStatus(inFlight ? L"Configuring..." : L"Load flight...");
			loadingPage.DrawAnimation();

			AdvanceToUpcomingTick(time, WelcomeAnimationIval, TimePoint::clock::now());
			device.ProcessMessages(time);

			// TODO: FSClient method?
			int re = 3;
			while (client.Receive(time) && re-- > 0);
			DBG_ASSERT (re);
		}
		while (CanReload(client) && (!inFlight || !config.IsReady()));
	}


#pragma endregion



#pragma region Init+Reset

	MfdLoop::MfdLoop(const volatile bool& uninterruptedFlag, const char* fSClientName, const FSTypeMapping& mapping) :
		FSClientName  { fSClientName },
		uninterrupted { uninterruptedFlag },
		typeMapping   { mapping }
	{
	}


	static void AddPages(FSPageList& list, X52Output& device)
	{
		if (list.Pages().empty())
			return;

		auto it = list.Pages().begin();
		SimPage& first = **it++;
		device.AddPage(first, /*activate:*/ true);

		for (; it != list.Pages().end(); ++it)
			device.AddPage(**it, false);
	}


	void MfdLoop::Run(X52Output& device)
	{
		// shouldn't exceed, but care not to display irrelevant data :)
		const Duration contentAgeLimit = BasePeriod / UpdateFreq * 2;

		while (CanUse(device))
		{
			FSClient client = ConnectToFS(device);
			if (!CanUse(device))
				return;

			try {
				codeAircraftLoaded = client.SubscribeEvent("AircraftLoaded", *this);
				codeInFlight       = client.SubscribeDetectInFlight(*this);

				Configurator config { client };

				while (CanReload(client))
				{
					WaitForFlight(device, client, config);
					if (!CanReload(client))					// interrupt or conn lost
						break;
					do
					{
						aircraftChanged = false;
						DBG_ASSERT (CanFlyAircraft(client) && config.IsReady());

						client.ResetVarGroups();

						FSPageList fsPages = config.CreatePages({ client, contentAgeLimit });
						AddPages(fsPages, device);
						
						LedControl ledControl { device, client, config.CreateLedEffects() };
						ledControl.ApplyDefaults();

						PollFS(device, ledControl, client);
					}
					while (CanFlyAircraft(client));

					config.Refresh();
				}
			}
			catch (const SimConnectError& ex)
			{
				// In case of any communication error with FS,
				// can just rerun this loop.
				Utils::FormatFlagScope { std::cerr }
					<< '\n' << ex.what()
					<< "\n(Error code: 0x" << std::hex << ex.error
					<< ")\nReconnecting..." << std::endl;
			}
		}
	}


	FSClient MfdLoop::ConnectToFS(X52Output& device)
	{
		Pages::WaitSpinner welcomePage { L"FS20-SaiMFD", 0, L"<-->" };
		welcomePage.SetStatus(L"Waiting for FS..");
		device.AddPage(welcomePage);

		FSClient client { FSClientName, typeMapping };

		TimePoint nextCheck = TimePoint::clock::now();
		while (CanUse(device) && !client.TryConnect())
		{
			AdvanceToUpcomingTick(nextCheck, WelcomeAnimationIval, TimePoint::clock::now());
			device.ProcessMessages(nextCheck);
			welcomePage.DrawAnimation();
		}
		return client;
	}

#pragma endregion



#pragma region Page Execution

	void MfdLoop::PollFS(X52Output& device, LedControl& leds, FSClient& client)
	{
		LOGIC_ASSERT (Duration::zero() < HotReceiveDelay && HotReceiveDelay < BasePeriod / FSPollFreq);

		const TimePoint init = TimePoint::clock::now();

		SynchClock nextAnimation { BasePeriod, AnimationFreq, init };
		SynchClock nextUpdate	 { BasePeriod, UpdateFreq,    init };
		SynchClock nextReceive	 { BasePeriod, FSPollFreq,    init };	// page-independent

		auto receiveSimBatch = [&](TimePoint now)
		{
			nextReceive.Advance(now);
			
			// try to empty SimConnect queue, but stay responsive
			// - assuming FSPollIval matches subscribed update freq. of SimVars
			int re = 5;
			while (client.Receive(now) && re-- > 0);
			
			if (re == 0)
				Debug::Warning("SimConnect queue filling up!");
		};
		
		const unsigned  syncingPollCycles  = (nextUpdate.Interval - HotReceiveDelay) / HotReceiveDelay;
		const unsigned  responsePollCycles = 0.35 * nextReceive.Interval / HotReceiveDelay;
		const SimPage*	hotPollingPage = nullptr;
		unsigned		hotPollsRemain = 0;

		bool devicePressed = false;
		while (CanFlyAircraft(client) && (devicePressed || CanUse(device)))
		{
			const TimePoint now = TimePoint::clock::now();
			auto* const actPage = static_cast<SimPage*>(device.GetActivePage());

			// 0. Wait for a Page if none active
			//	  Keep listening for sys events
			if (actPage == nullptr)
			{
				// LEDs can't be operated when plugin doesn't own the active page!
				leds.Disable();

				if (nextReceive.IsDue(now))
					receiveSimBatch(now);

				devicePressed = device.ProcessNextMessage(nextReceive.Tick());
				continue;
			}
			leds.Enable();

			// 1. Dispatch/Receive FS notifications
			//	  Expedite it when Page needs it (Page change / SetSimvar caused by input)
			const bool cleanPoll = hotPollingPage != actPage;
			const bool hotPoll	 = actPage->IsAwaitingReceive() && (cleanPoll || hotPollsRemain);
			if (hotPoll)
			{
				Debug::Info("MfdLoop", "Hot-polling FS...");

				const bool sync = actPage->IsAwaitingData();
				hotPollsRemain = cleanPoll && actPage->IsAwaitingData()		? syncingPollCycles
							   : cleanPoll && actPage->IsAwaitingResponse()	? responsePollCycles
							   : hotPollsRemain - 1;
				
				DBG_ASSERT (hotPollsRemain <= std::max(syncingPollCycles, responsePollCycles));
				receiveSimBatch(now);
				
				// adjust sync after Page-change
				if (sync && !actPage->IsAwaitingData())
				{
					// TODO: No LED update on failed sync. Try to refactor this anyway.
					leds.ApplyUpdate();
					auto delayed = std::chrono::duration_cast<std::chrono::milliseconds>((syncingPollCycles - hotPollsRemain) * HotReceiveDelay);
					Debug::Info("MfdLoop", "Synced polling with FS. Delayed [ms]:", delayed.count());
					nextReceive.Reset(now, true);		// just done
					nextAnimation.Reset(now, true);		// don't animate immediately
					nextUpdate.Reset(now);
				}
			}
			else if (nextReceive.IsDue(now))
			{
				receiveSimBatch(now);
				leds.ApplyUpdate();
			}
			hotPollingPage = actPage->IsAwaitingReceive() ? actPage : nullptr;

			// 1.5 Drive LEDs (if receive hasn't triggered them already)
			leds.Blink(now);

			// 2. Signal that an Update is due (might not received)
			if (nextUpdate.IsDue(now))
			{
				nextUpdate.Advance(now);
				actPage->Update(now);
			}

			// 3. Animate
			if (nextAnimation.IsDue(now))
			{
				unsigned ticksPassed = nextAnimation.Advance(now);
				actPage->Animate(ticksPassed);
			}

			// 4. End "page cycle"
			actPage->DrawLines();

			TimePoint deadline = hotPoll 
				? now + HotReceiveDelay
				: Utils::Min<TimePoint>(nextReceive, nextAnimation, nextUpdate, leds.NextBlinkTime());
			devicePressed = device.ProcessNextMessage(deadline);
		}
	}

#pragma endregion


}	//namespace FSMfd
