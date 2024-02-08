#include "FSClient.h"

#include "SimConnectError.h"
#include "ConfigHelper.h"
#include "Utils/BasicUtils.h"
#include "Utils/Debug.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SimConnect.h"

#include <sstream>
#include <algorithm>


#define INMOCK_RETURN		if (MockMode)	return
#define FS_REQUEST(call)	if (!MockMode) { FS_ASSERT(call); }


namespace FSMfd::SimClient 
{
	constexpr char LogSource[] = "FSCLient";



#pragma region Connection

	FSClient::FSClient(const char* appName, const FSTypeMapping& mapping) :
		ClientAppName { appName },
		TypeMapping	  { mapping },
		hSimConnect	  { nullptr }
	{
		LOGIC_ASSERT_M (std::find(mapping.begin(), mapping.end(), 0) == mapping.end(),
						"Unspecified SimConnect type for FSClient?"					 );
	}


	FSClient::~FSClient()
	{
		if (hSimConnect)
		{
			HRESULT hr = SimConnect_Close(hSimConnect);
			DBG_ASSERT_M (SUCCEEDED(hr), "Failed to close SimConnect session.");
		}
	}


	bool FSClient::TryConnect()
	{
		INMOCK_RETURN true;

		if (hSimConnect)	// NOTE: Errors wont close it, only the dtor!
			return true;

		HRESULT hr = SimConnect_Open(&hSimConnect, ClientAppName, nullptr,
									 0, 0, SIMCONNECT_OPEN_CONFIGINDEX_LOCAL);
		return SUCCEEDED(hr);
	}


	bool FSClient::IsConnected() const
	{
		INMOCK_RETURN true;

		return hSimConnect != nullptr;
	}

#pragma endregion



#pragma region SimVar Handling

	static size_t GetLengthDword(SIMCONNECT_DATATYPE typ)
	{
		size_t bytes = GetSizeOf(typ);
		DBG_ASSERT (bytes % 4 == 0);

		return bytes / 4;
	}


	FSClient::VarGroup::VarGroup() :
		varPositions { 0 },
		dataReceiver { nullptr },
		oneTime		 { false }
	{
	}


	bool FSClient::VarGroup::IsEmpty() const
	{
		DBG_ASSERT (VarCount() > 0 || dataReceiver == nullptr);

		return VarCount() == 0;
	}


	VarIdx FSClient::VarGroup::VarCount() const
	{
		DBG_ASSERT (!varPositions.empty());

		return Implied<VarIdx>(varPositions.size()) - 1;
	}


	size_t FSClient::VarGroup::ExpectedBytes() const
	{
		return varPositions.back() * sizeof(DWORD);
	}


	void FSClient::VarGroup::Add(SIMCONNECT_DATATYPE typ)
	{
		size_t dwords = GetLengthDword(typ);

		varPositions.push_back(varPositions.back() + dwords);
	}


	VarIdx FSClient::AddVar(GroupId gid, const SimVarDef& vardef)
	{
		VarGroup&	 group = AccessGroup(gid);
		const VarIdx idx = group.VarCount();
		
		LOGIC_ASSERT_M (idx < MaxVarsPerGroup, "Too many variables added to single group!");

		SIMCONNECT_DATATYPE typ = TypeMapping[AsIndex(vardef.typeReqd)];
		
		FS_REQUEST (
			SimConnect_AddToDataDefinition(hSimConnect,
										   ToSimId(gid),
										   vardef.name.c_str(),
										   vardef.unit.c_str(),
										   typ				  )
		);
		group.Add(typ);
		return idx;
	}

#pragma endregion




#pragma region GroupId Handling

	static bool IsPermanent(GroupId id) noexcept
	{
		return id < MaxPermanentGroups;
	}


	static size_t ToIndex(GroupId id) noexcept
	{
		bool shifts = !IsPermanent(id);
		return id - shifts * MaxPermanentGroups;
	}


	uint32_t FSClient::ToSimId(GroupId gid) const noexcept
	{
		bool shifts = !IsPermanent(gid);
		return gid + shifts * exhaustedGroupIdCount;
	}


	GroupId FSClient::ToGroupId(uint32_t simId) const noexcept
	{
		bool shifts = !IsPermanent(simId);
		return simId - shifts * exhaustedGroupIdCount;
	}


	auto FSClient::AccessGroup(GroupId gid) -> VarGroup&
	{
		VarGroup* g = TryAccessGroup(gid);

		LOGIC_ASSERT_M (g != nullptr, "Undefined variable group!");
		return *g;
	}


	auto FSClient::TryAccessGroup(GroupId gid) noexcept -> VarGroup*
	{
		auto&	arr = IsPermanent(gid) ? varGroupsPermanent : varGroups;
		size_t	idx = ToIndex(gid);
		if (idx >= arr.size())
			return nullptr;

		return &arr[idx];
	}

	
	GroupId FSClient::CreateVarGroup(GroupLifetime life)
	{
		if (life == GroupLifetime::Resettable)
		{
			GroupId next = MaxPermanentGroups + Practically<GroupId>(varGroups.size());
			varGroups.emplace_back();
			return next;
		}
		DBG_ASSERT (life == GroupLifetime::Permanent);

		GroupId next = Implied<GroupId>(varGroupsPermanent.size());
		LOGIC_ASSERT_M (next < MaxPermanentGroups, "Permanent Simvar groups exhausted.");
		varGroupsPermanent.emplace_back();
		return next;
	}

#pragma endregion




#pragma region VarGroup Activation

	void FSClient::RequestOnetimeUpdate(GroupId gid, IDataReceiver& receiver)
	{
		VarGroup& group = AccessGroup(gid);

		LOGIC_ASSERT_M (group.dataReceiver == nullptr ||
						group.dataReceiver == &receiver && group.oneTime,
						"Already have a different subscription!"	    );

		DWORD simId = ToSimId(gid);

		FS_REQUEST (
			SimConnect_RequestDataOnSimObject(hSimConnect, simId, simId,
											  SIMCONNECT_OBJECT_ID_USER,
											  SIMCONNECT_PERIOD_ONCE, 0, 0, 0)
		);
		group.dataReceiver = &receiver;
		group.oneTime = true;

		// Not a continuous load!
		//++subscriptionCount;
	}


	void FSClient::EnableVarGroup(GroupId gid, IDataReceiver& reciever, UpdateFrequency freq)
	{
		VarGroup& group = AccessGroup(gid);

		LOGIC_ASSERT_M (group.dataReceiver == nullptr, "Already subscribed to group!");

		SIMCONNECT_PERIOD update = (freq == UpdateFrequency::PerSecond)
			? SIMCONNECT_PERIOD_SECOND
			: SIMCONNECT_PERIOD_VISUAL_FRAME;

		SIMCONNECT_DATA_REQUEST_FLAG flags = (freq == UpdateFrequency::OnValueChange) 
			? SIMCONNECT_DATA_REQUEST_FLAG_CHANGED
			: 0;

		DWORD interval = (freq == UpdateFrequency::FrameDriven) ? 6 : 0;		// ~30 FPS / 6 --> 4..5Hz
		DWORD	 simId = ToSimId(gid);

		FS_REQUEST (
			SimConnect_RequestDataOnSimObject(hSimConnect, simId, simId,
											  SIMCONNECT_OBJECT_ID_USER,
											  update, flags, 0, interval)
		);
		group.dataReceiver = &reciever;
		++subscriptionCount;
	}


	void FSClient::DisableVarGroup(GroupId gid)
	{
		VarGroup& group = AccessGroup(gid);
		
		LOGIC_ASSERT_M (group.dataReceiver != nullptr, "Not subscribed to group!");
		
		DWORD simId = ToSimId(gid);

		FS_REQUEST (
			SimConnect_RequestDataOnSimObject(hSimConnect, simId, simId,
											  SIMCONNECT_OBJECT_ID_USER,
											  SIMCONNECT_PERIOD_NEVER  )
		);
		group.dataReceiver = nullptr;
		--subscriptionCount;
	}


	void FSClient::ClearVarGroup(GroupId gid)
	{
		VarGroup& group = AccessGroup(gid);
		DWORD	  simId = ToSimId(gid);

		FS_REQUEST (SimConnect_ClearClientDataDefinition(hSimConnect, simId));

		if (group.dataReceiver != nullptr)
			--subscriptionCount;
		group = {};
	}


	bool FSClient::TryClearVarGroup(GroupId gid) noexcept
	{
		VarGroup* const group = TryAccessGroup(gid);
		if (group == nullptr)
		{
			DBG_BREAK;
			return false;
		}

		
		INMOCK_RETURN *group = {}, true;

		const DWORD simId = ToSimId(gid);
		if (group->dataReceiver)
		{
			HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect, simId, simId,
														   SIMCONNECT_OBJECT_ID_USER,
														   SIMCONNECT_PERIOD_NEVER  );
			DBG_ASSERT(SUCCEEDED(hr));
		}
		HRESULT hr = SimConnect_ClearClientDataDefinition(hSimConnect, simId);
		
		// Deleting nonexistent probably can yield a FAILURE
		const bool succ = SUCCEEDED(hr) || group->VarCount() == 0;
		DBG_ASSERT(succ);
		
		if (group->dataReceiver != nullptr)
			--subscriptionCount;
		
		*group = {};
		return succ;
	}


	void FSClient::ResetVarGroups()
	{
		GroupId id = MaxPermanentGroups;
		for (VarGroup& gr : varGroups)
		{
			if (!gr.IsEmpty())
				ClearVarGroup(id);
			id++;
		}
		exhaustedGroupIdCount += Practically<GroupId>(varGroups.size());
	}

#pragma endregion


	

#pragma region InFlightDetector - workaround

	class FSClient::InFlightDetector : public IEventReceiver {
		FSClient*				owner;
		NotificationCode		codeFlightLoad;
		NotificationCode		codeSimRunning;
		bool					simActive	   = false;
		bool					mainMenuLoaded = false;		// TODO: if started already in Main Menu

	public:
		const NotificationCode	DetectionEvent;

		bool InFlight() const	{ return simActive && !mainMenuLoaded; }


		InFlightDetector(FSClient& owner) :
			owner		   { &owner },
			DetectionEvent { owner.nextEventId++ }
		{
			try
			{
				codeFlightLoad = owner.SubscribeEvent("FlightLoaded", *this);
				codeSimRunning = owner.SubscribeEvent("Sim", *this);
			}
			catch (...)
			{
				owner.UnscribeEvents(*this);
				throw;
			}
		}

		InFlightDetector(InFlightDetector&&) = delete;

		void OwnerMoved(FSClient& newAddr) { owner = &newAddr; }

	private:
		void Notify(NotificationCode code, uint32_t parameter, TimePoint stamp) override
		{
			LOGIC_ASSERT (code == codeSimRunning);

			bool oldInFlight = InFlight();
			simActive = static_cast<bool>(parameter);
			if (oldInFlight != InFlight())
				PushChange(stamp);
		}


		void Notify(NotificationCode code, const char* cpath, TimePoint stamp) override
		{
			LOGIC_ASSERT (code == codeFlightLoad);

			constexpr std::string_view MenuTrail = "MainMenu.FLT";

			std::string_view path { cpath };

			bool toMenu = false;
			if (MenuTrail.length() < path.length())
			{
				size_t pos = path.length() - MenuTrail.length();
				toMenu = _stricmp(&path[pos], MenuTrail.data()) == 0;
			}

			bool oldInFlight = InFlight();
			mainMenuLoaded = toMenu;

			if (InFlight() != oldInFlight)
				PushChange(stamp);
		}


		void PushChange(TimePoint stamp)
		{
			owner->PushEvent(stamp, DetectionEvent, InFlight());
		}
	};



	FSClient::FSClient(FSClient&& src) noexcept :
		hSimConnect           { src.hSimConnect },
		ClientAppName         { src.ClientAppName },
		TypeMapping           { src.TypeMapping },
		nextEventId           { src.nextEventId },
		subscriptionCount     { src.subscriptionCount },
		exhaustedGroupIdCount { src.exhaustedGroupIdCount },
		inflightDetector	  { std::move(src.inflightDetector) },
		varGroups             (std::move(src.varGroups)),
		eventSubscribers      (std::move(src.eventSubscribers))
	{
		if (inflightDetector != nullptr)
			inflightDetector->OwnerMoved(*this);
	}

#pragma endregion




#pragma region Push event/data

	void FSClient::PushData(TimePoint stamp, GroupId gid, VarGroup& trg, const uint32_t* data)
	{
		if (trg.dataReceiver != nullptr)
		{
			SimvarList vals { trg.varPositions, data };
			trg.dataReceiver->Receive(gid, vals, stamp);
			
			if (trg.oneTime)
			{
				trg.dataReceiver = nullptr;
				trg.oneTime = false;
			}
		}
	}


	void FSClient::PushEvent(TimePoint stamp, NotificationCode code, uint32_t parameter)
	{
		for (EventSubscriber& es : eventSubscribers)
		{
			if (es.code == code)
				es.subscriber.Notify(code, parameter, stamp);
		}
	}


	void FSClient::PushStringEvent(TimePoint stamp, NotificationCode code, const char* str)
	{
		for (EventSubscriber& es : eventSubscribers)
		{
			if (es.code == code)
				es.subscriber.Notify(code, str, stamp);
		}
	}

#pragma endregion




#pragma region Event Subscriptions

	NotificationCode FSClient::SubscribeDetectInFlight(IEventReceiver& receiver)
	{
		if (inflightDetector == nullptr)
			inflightDetector = std::make_unique<InFlightDetector>(*this);
	
		NotificationCode code = inflightDetector->DetectionEvent;
		eventSubscribers.emplace_back(code, receiver);
		return code;
	}


	uint32_t FSClient::SubscribeEvent(const char* name, IEventReceiver& receiver)
	{
		FS_REQUEST (
			SimConnect_SubscribeToSystemEvent(hSimConnect, nextEventId, name)
		);
		uint32_t code = nextEventId++;
		eventSubscribers.emplace_back(code, receiver);
		++subscriptionCount;
		return code;
	}


	void FSClient::UnscribeEvents(IEventReceiver& receiver)
	{
		bool inFlightDetectInvolved = false;

		auto it = eventSubscribers.begin();
		while (it != eventSubscribers.end())
		{
			if (&(*it)->subscriber == &receiver)
			{
				if (inflightDetector == nullptr || (*it)->code != inflightDetector->DetectionEvent)
				{
					FS_REQUEST (
						SimConnect_UnsubscribeFromSystemEvent(hSimConnect, (*it)->code)
					);
					--subscriptionCount;
				}
				else
				{
					inFlightDetectInvolved = true;
				}
				it = eventSubscribers.erase(it);
			}
			else
			{
				++it;
			}
		}

		bool inflightUnused = inFlightDetectInvolved &&
							  !Utils::AnyOf(eventSubscribers, [this](auto& sub) {
								return sub->code == inflightDetector->DetectionEvent;
							  });
		if (inflightUnused)
		{
			UnscribeEvents(*inflightDetector);
			inflightDetector.reset();
		}
	}

#pragma endregion




#pragma region Receive

	// Context for SimConnect CALLBACK
	// + performs basic message validations involving VarGroup data
	struct FSClient::ReceiveContext {
		FSClient&		self;
		TimePoint		stamp;
		bool			received = false;		// to signal "correct" nothing without error
		bool			quit	 = false;		// explicit quit signal received
		const char*		error    = nullptr;
		const char*		warning  = nullptr;
		optional<DWORD>	receivedException;


		template <class Fun, class... Args>
		void Invoke(Fun push, Args&&... args)
		{
			try	// guard exceptions, running under SimConnect CALLBACK here!
			{
				(self.*push)(stamp, args...);
			}
			catch (const std::exception& ex)
			{
				error = ex.what();
			}
			catch (...)
			{
				error = "Unknown exception.";
			}
		}


		void HandleData(const SIMCONNECT_RECV_SIMOBJECT_DATA& objData, DWORD count)
		{
			const DWORD simId = objData.dwRequestID;

			bool lateMsg = !IsPermanent(simId) &&
						   simId < self.exhaustedGroupIdCount + MaxPermanentGroups;
			if (lateMsg)
			{
				Debug::Info(LogSource, "Received late message for now obsolete group.");
				return;
			}

			const GroupId gid   = self.ToGroupId(simId);
			VarGroup*	  group = self.TryAccessGroup(gid);

			bool groupOk = group != nullptr && objData.dwDefineID == simId;
			bool seqOk   = (objData.dwentrynumber == 1) && (objData.dwoutof == 1);
			if (!groupOk || !seqOk)
			{
				error = "Received invalid or inconsistent message.";
				return;
			}
			if (objData.dwDefineCount != group->VarCount())
			{
				if (group->IsEmpty())
					warning = "Received obsolete packet for inactivated VarGroup.";
				else
					error	= "Received unexpected data count. Check configured SimVar identifiers and their supported units.";
				return;
			}
			// not so important check - objData ends in a flexible array
			DBG_ASSERT(count == group->ExpectedBytes() + sizeof(SIMCONNECT_RECV_SIMOBJECT_DATA) - sizeof(objData.dwData));

			// just to avoid including SimConnect everywhere
			static_assert(sizeof(uint32_t) == sizeof(DWORD));
			const uint32_t* data = reinterpret_cast<const uint32_t*>(&objData.dwData);
		
			Invoke(&FSClient::PushData, gid, *group, data);
		}

		
		bool CheckIsSysEvent(const SIMCONNECT_RECV_EVENT& eventData)
		{
			bool ok = eventData.uGroupID == SIMCONNECT_RECV_EVENT::UNKNOWN_GROUP
				   || eventData.uGroupID == 0;

			if (ok)
				return true;

			warning = "Ignoring client-defined (3rd-party?) event.";
			return false;
		}


		void HandleEvent(const SIMCONNECT_RECV_EVENT& eventData, DWORD count)
		{
			if (!CheckIsSysEvent(eventData))
				return;

			NotificationCode code = eventData.uEventID;
			Invoke(&FSClient::PushEvent, code, eventData.dwData);
		};


		void HandleEvent(const SIMCONNECT_RECV_EVENT_FILENAME& eventData, DWORD count)
		{
			if (!CheckIsSysEvent(eventData))
				return;

			NotificationCode code = eventData.uEventID;
			Invoke(&FSClient::PushStringEvent, code, eventData.szFileName);
		};


		void HandleOpen(const SIMCONNECT_RECV_OPEN& ack)
		{
			if (self.simConnectVer.has_value())
				warning = "Received duplicated Open packet.";

			self.simConnectVer = VersionNumber {
				{ ack.dwSimConnectVersionMajor, ack.dwSimConnectVersionMinor },
				{ ack.dwSimConnectBuildMajor,   ack.dwSimConnectBuildMinor }  
			};
		}


		void HandleReceivedException(const SIMCONNECT_RECV_EXCEPTION& exData)
		{
			receivedException = exData.dwException;

			// friendly message for errors commonly caused by wrong configuration
			switch (exData.dwException)
			{
				case SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID:
					warning = "SimConnect exception: \tUnrecognized ID!";
					break;
				case SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED:
					warning = "SimConnect exception: \tUnrecognized parameter name!";
					break;
				case SIMCONNECT_EXCEPTION_INVALID_ENUM:
					warning = "SimConnect exception: \tInvalid Enum!";
					break;
				case SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE:
					warning = "SimConnect exception: \tInvalid Type!";
					break;
			}
		}

		// no mixing with other warnings currently
		bool HasInterpretedException() const	{ return receivedException.has_value(); }


		void FSQuit()
		{
			quit = true;
			self.hSimConnect = nullptr;
			self.simConnectVer.reset();
			Debug::Warning(LogSource, "FS Exiting.");
		}
	};


	static void CALLBACK ProcessSimMessage(SIMCONNECT_RECV* pData, DWORD byteCount, void* pContext)
	{
		DBG_ASSERT (pContext != nullptr);

		auto* context = reinterpret_cast<FSClient::ReceiveContext*> (pContext);

		if (!pData || pData->dwID == SIMCONNECT_RECV_ID_NULL)
			return;

		context->received = true;
		switch (pData->dwID)
		{
			case SIMCONNECT_RECV_ID_OPEN:
				context->HandleOpen(*static_cast<const SIMCONNECT_RECV_OPEN*>(pData));
				return;
			
			case SIMCONNECT_RECV_ID_QUIT:
				context->FSQuit();
				return;

			case SIMCONNECT_RECV_ID_EVENT:
			case SIMCONNECT_RECV_ID_EVENT_FRAME:
			case SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE:
			{
				context->HandleEvent(*static_cast<SIMCONNECT_RECV_EVENT*> (pData), byteCount);
				return;
			}
			case SIMCONNECT_RECV_ID_EVENT_FILENAME:
			{
				context->HandleEvent(*static_cast<SIMCONNECT_RECV_EVENT_FILENAME*> (pData), byteCount);
				return;
			}

			case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
			{
				context->HandleData(*static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*> (pData), byteCount);
				return;
			}

			case SIMCONNECT_RECV_ID_EXCEPTION:
			{
				context->HandleReceivedException(*static_cast<SIMCONNECT_RECV_EXCEPTION*> (pData));
				return;
			}

			default:
				context->warning = "Received something unknown...";
		}
	}


	bool FSClient::MockReceive(TimePoint now)
	{
		constexpr Duration  Wait          = 2s;
		constexpr Duration  FirstDataWait = 300ms;
		static	  TimePoint firstTime = now;
		static	  TimePoint lastTime  = now;
		static	  bool		connected = false;

		if (now < lastTime + Wait)
			return false;

		// Simulate connection
		if (!connected && firstTime + Wait <= now)
		{
			PushEvent(now, inflightDetector->DetectionEvent, 1u);
			// just guess the first subscriber here...
			PushStringEvent(now, eventSubscribers[0]->code, "SomeNotMenuAircraft.FLT");
			lastTime = now + FirstDataWait - Wait;
			connected = true;
			return true;
		}

		auto addition = static_cast<unsigned>(std::chrono::duration_cast<std::chrono::seconds>(now - firstTime).count());

		auto pushMockData = [this, addition, now](GroupId gid)
		{
			VarGroup& g = AccessGroup(gid);
			if (g.dataReceiver == nullptr)
				return;

			std::vector<uint32_t> data (g.ExpectedBytes() / sizeof(uint32_t), 0);

			for (VarIdx i = 0; i < g.VarCount(); i++)
			{
				size_t pos = g.varPositions[i];
				size_t len = g.varPositions[i + 1] - pos;
				if (len == 1)
					data[pos] = 110 * (1 + i) + addition;
				else
					reinterpret_cast<double&>(data[pos]) = 10.0 * i + 0.01 * addition;
			}

			PushData(now, gid, g, data.data());
		};

		for (GroupId gid = 0; gid < varGroupsPermanent.size(); gid++)
			pushMockData(gid);

		for (GroupId i = 0; i < varGroups.size(); i++)
			pushMockData(i + MaxPermanentGroups);

		bool duringConfig = varGroups.empty();

		lastTime = duringConfig ? (now + FirstDataWait - Wait) : now;
		return true;
	}


	bool FSClient::Receive(TimePoint now)
	{
		INMOCK_RETURN MockReceive(now);

		ReceiveContext context { *this, now };

		// TODO: Disconnect: error = 0xc00000b0 : The specified named pipe is in the disconnected state.
		//					 error = 0xc000014b : Broken pipe 
		HRESULT hr = SimConnect_CallDispatch(hSimConnect, &ProcessSimMessage, &context);
		if (context.warning)
			Debug::Warning(LogSource, context.warning);
		
		if (context.receivedException && !context.HasInterpretedException())
			Debug::Warning(LogSource, "SimConnect exception: ", *context.receivedException);

		LOGIC_ASSERT_M (context.error == nullptr, context.error);
		
		if (!context.quit)
			FS_ASSERT (hr);

		return !context.quit && context.received;
	}


	bool FSClient::ReceiveMultiple(TimePoint now, unsigned tries)
	{
		while (tries && Receive(now))
			--tries;

		return tries == 0;
	}

#pragma endregion


}	// namespace FSMfd
