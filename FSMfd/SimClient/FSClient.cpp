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
	using Utils::AsIndex;




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

		return varPositions.size() - 1;
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
			SimConnect_AddToDataDefinition(hSimConnect, ToSimId(gid), vardef.name, vardef.unit, typ)
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
			GroupId next = MaxPermanentGroups + varGroups.size();
			varGroups.emplace_back();
			return next;
		}
		DBG_ASSERT (life == GroupLifetime::Permanent);

		GroupId next = varGroupsPermanent.size();
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


	void FSClient::EnableVarGroup(GroupId gid, IDataReceiver& reciever, bool fastUpdate)
	{
		VarGroup& group = AccessGroup(gid);

		LOGIC_ASSERT_M (group.dataReceiver == nullptr, "Already subscribed to group!");

		SIMCONNECT_PERIOD update = fastUpdate
			? SIMCONNECT_PERIOD_VISUAL_FRAME 
			: SIMCONNECT_PERIOD_SECOND;
		
		DWORD interval = fastUpdate ? 6 : 0;		// fast: ~30 FPS / 6 --> 4..5Hz
		DWORD	 simId = ToSimId(gid);

		FS_REQUEST (
			SimConnect_RequestDataOnSimObject(hSimConnect, simId, simId,
											  SIMCONNECT_OBJECT_ID_USER,
											  update, 0, 0, interval   )
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
		exhaustedGroupIdCount += varGroups.size();
	}

#pragma endregion


	

#pragma region InFlightDetector - workaround

	class FSClient::InFlightDetector : public IEventReceiver
	{
		FSClient*				owner;
		NotificationCode		codeFlightLoad;
		NotificationCode		codeSimRunning;
		bool					simActive	   = false;
		bool					mainMenuLoaded = false;		// TODO: if started already in Main Menu

	public:
		const NotificationCode	ForwardingCode;	// TODO: naming?

		bool InFlight() const	{ return simActive && !mainMenuLoaded; }

		
		void OwnerMoved(FSClient& newAddr) { owner = &newAddr; }

		InFlightDetector(InFlightDetector&&) = delete;

		InFlightDetector(FSClient& owner) :
			owner		   { &owner },
			ForwardingCode { owner.nextEventId++ }
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
			owner->PushEvent(stamp, ForwardingCode, InFlight());
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
	
		NotificationCode code = inflightDetector->ForwardingCode;
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
				if (inflightDetector == nullptr || (*it)->code != inflightDetector->ForwardingCode)
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

		// TODO: a counter maybe?
		if (inFlightDetectInvolved)
		{
			auto other = std::find_if(eventSubscribers.begin(), eventSubscribers.end(), [this](auto& sub) {
				return sub->code == inflightDetector->ForwardingCode;
			});

			if (other == eventSubscribers.end())
			{
				UnscribeEvents(*inflightDetector);
				inflightDetector.reset();
			}
		}
	}

#pragma endregion




#pragma region Receive

	// Context for SimConnect CALLBACK
	// + performs basic message validations involving VarGroup data
	struct FSClient::ReceiveContext {
		FSClient&	self;
		TimePoint	stamp;
		bool		received = false;		// to signal "correct" nothing without error
		bool		quit	 = false;		// explicit quit signal received
		const char* error    = nullptr;
		const char* warning  = nullptr;


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
				return;

			// TODO: ToLogical / ToSimId etc...
			//const bool shiftingId = !IsPermanent(simId);
			//const GroupId gid = simId - shiftingId * self.exhaustedGroupIdCount;
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
					error	= "Received unexpected data count.";
				return;
			}
		// TODO
		//	DBG_ASSERT(count == self.varGroups[gid].ExpectedBytes());

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


		void FSQuit()
		{
			quit = true;
			self.hSimConnect = nullptr;
			Debug::Warning("SimClient", "FS Exiting.");
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
				return;

			case SIMCONNECT_RECV_ID_QUIT:
				context->FSQuit();
				return;

			case SIMCONNECT_RECV_ID_EVENT:
			case SIMCONNECT_RECV_ID_EVENT_FRAME:
			case SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE:
			{
				auto* pEvntData = static_cast<SIMCONNECT_RECV_EVENT*> (pData);

				context->HandleEvent(*pEvntData, byteCount);
				return;
			}
			case SIMCONNECT_RECV_ID_EVENT_FILENAME:
			{
				auto* pEvntData = static_cast<SIMCONNECT_RECV_EVENT_FILENAME*> (pData);
				context->HandleEvent(*pEvntData, byteCount);
				return;
			}

			case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
			{
				auto* pObjData = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*> (pData);

				context->HandleData(*pObjData, byteCount);
				return;
			}

			// TODO
			case SIMCONNECT_RECV_ID_EXCEPTION:
			{
				auto* pExData = static_cast<SIMCONNECT_RECV_EXCEPTION*> (pData);
				std::stringstream ss;
				ss << "SimConnect exception: " << pExData->dwException;
				Debug::Warning("FSClient", ss.str().data());
				return;
			}

			default:
				context->warning = "Received something unknown...";
		}
	}


	bool FSClient::MockReceive(TimePoint now)
	{
		constexpr Duration  Wait = 500ms;
		static	  TimePoint firstTime = now;
		static	  TimePoint lastTime  = now;

		if (now < lastTime + Wait)
			return false;

		unsigned short addition = std::chrono::duration_cast<std::chrono::seconds>(now - firstTime).count();

		for (GroupId gid = 0; gid < varGroups.size(); gid++)
		{
			const VarGroup& g = varGroups[gid];
			if (g.dataReceiver == nullptr)
				continue;

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

		// TODO Mock
		//	PushData(now, gid, data.data());
		}
		lastTime = now;
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
			Debug::Warning("FSCLient", context.warning);
		
		LOGIC_ASSERT_M (context.error == nullptr, context.error);
		
		if (!context.quit)
			FS_ASSERT (hr);

		return !context.quit && context.received;
	}

#pragma endregion



}	// namespace FSMfd


