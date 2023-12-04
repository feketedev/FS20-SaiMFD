#pragma once

	/*  Part of FS20-SaiMFD			   *
	 *  Copyright 2023 Norbert Fekete  *
     *                                 *
	 *  Released under GPLv3.		   */


#include "IReceiver.h"
#include "FSClientTypes.h"
#include "Utils/Reassignable.h"

#include <array>
#include <memory>



namespace FSMfd::SimClient {

	/// Wether ResetVarGroups will affect the Group or not.
	enum class GroupLifetime { Resettable, Permanent };


	// according to SimConnect SDK
	static constexpr unsigned MaxVarsPerGroup = 1000;

	// SimConnect_ClearClientDataDefinition workaround:
	// "Cleared" groups cannot be reused with FS for some reason
	// -> split GroupId-space to Permanent-Resettable parts
	// -> internally every Reset shifts Resettable Id-space
	static constexpr unsigned MaxPermanentGroups = 256;




	///	  Wrapper around MSFS2020 SimConnect.
	/// @remarks
	///	  Each communicating method throws SimConnectError on FAILURE.
	///	  In such case it's best to discard this whole object and reconnect.
	///	  (Error codes could be checked for NTSTATUSes denoting disconnection
	///	   [STATUS_REMOTE_DISCONNECT, ...?], not being done currently.)
	class FSClient {

		void*	hSimConnect;


		// Each group represents a SimConnect DataDefinition, as well as a DataRequest.
		struct VarGroup {
			std::vector<size_t>		varPositions;	// last denotes end of data buffer
			IDataReceiver*			dataReceiver;
			bool					oneTime;

			bool	IsEmpty()		const;
			VarIdx	VarCount()		const;
			size_t	ExpectedBytes() const;
			void	Add(SIMCONNECT_DATATYPE);
			VarGroup();
			VarGroup(VarGroup&&)				 = default;
			VarGroup& operator=(VarGroup&&)		 = default;
			VarGroup& operator=(const VarGroup&) = delete;
		};
		std::vector<VarGroup>	varGroupsPermanent;
		std::vector<VarGroup>	varGroups;
		GroupId					exhaustedGroupIdCount = 0;


		// FS System events
		struct EventSubscriber {
			NotificationCode	code;
			IEventReceiver&		subscriber;
		};
		std::vector<Utils::Reassignable<EventSubscriber>>	eventSubscribers;
		
		NotificationCode nextEventId	   = 1;					
		size_t			 subscriptionCount = 0;


		// an internal workaround on ambiguous events
		class InFlightDetector;
		std::unique_ptr<InFlightDetector>	inflightDetector;

	public:
		/// Pages Debug: set to run without Sim
		static constexpr bool MockMode = false;


		// ----- Fields + special -------------------------------------------------------

		const char* const	ClientAppName;
		const FSTypeMapping TypeMapping;

		/// Total count of expected kinds of messages during receive.
		size_t	SubscriptionCount() const   { return subscriptionCount; }

		FSClient(const char* appName, const FSTypeMapping&);
		FSClient(FSClient&&) noexcept;
		~FSClient();
		

		// ----- Simulation Variables ---------------------------------------------------

		GroupId	CreateVarGroup(GroupLifetime = GroupLifetime::Resettable);

		/// Add a SimVar to a group to be watched.
		/// @returns Index of the new variable within the group
		VarIdx AddVar(GroupId, const SimVarDef&);

		void RequestOnetimeUpdate(GroupId, IDataReceiver&);

		/// Register receiver for notifications about the given variable group.
		/// @param fastUpdate:	apprx. 5Hz depending on FPS, instead of PER_SECOND.
		void EnableVarGroup(GroupId, IDataReceiver&, bool fastUpdate = false);

		/// Disable notifications about variable group and unregister its receiver.
		void DisableVarGroup(GroupId);

		/// Disable and Clear variable definitions of a variable group.
		/// @remarks
		///	  Due to SimConnect quirks Cleared Ids cannot be reused until ResetVarGroups.
		void ClearVarGroup(GroupId);

		/// Same as ClearVarGroup - for easier exception-safety.
		bool TryClearVarGroup(GroupId) noexcept;

		void ResetVarGroups();

		
		// ----- Events -----------------------------------------------------------------

		/// Pseudo event with the best possible workaround.
		/// Received param is 1 entering, 0 exiting flight.
		NotificationCode	SubscribeDetectInFlight(IEventReceiver&);

		NotificationCode	SubscribeEvent(const char* name, IEventReceiver&);
		
		void				UnscribeEvents(IEventReceiver&);


		// ----- Connect + Run ----------------------------------------------------------

		bool TryConnect();
		bool IsConnected() const;

		/// Request FS to dispatch new values in Enabled variable groups and notify Receivers.
		/// 
		/// @param now:	stamp to be pushed to Receivers to avoid unnecessary clock::now calls.
		/// @returns:	more coming => should check again to keep the queue empty!
		bool [[nodiscard]] Receive(TimePoint now);
	
		/// Call Receive multiple times in attempt to empty the FS's queue. 
		/// @remarks
		///				It seems that SimConnect does dispatch all queued messages under a
		///				single request, only what's received in the mean-time will remain
		///				- hence the suggested default parameter.
		/// @returns:	even more coming
		bool ReceiveMultiple(TimePoint now, unsigned triesToEmpty = 3);

	
		struct ReceiveContext;		// internal

	private:
		void PushData(TimePoint, GroupId, VarGroup&, const uint32_t* data);
		void PushEvent(TimePoint, NotificationCode, uint32_t parameter);
		void PushStringEvent(TimePoint, NotificationCode, const char* path);

		bool MockReceive(TimePoint);
		
		uint32_t	ToSimId(GroupId gid)		const noexcept;
		GroupId		ToGroupId(uint32_t simId)	const noexcept;
		VarGroup&	AccessGroup(GroupId);
		VarGroup*	TryAccessGroup(GroupId)		noexcept;
	};


}	// namespace FSMfd::SimClient
