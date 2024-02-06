#pragma once

	/*  Part of FS20-SaiMFD			   *
	 *  Copyright 2023 Norbert Fekete  *
     *                                 *
	 *  Released under GPLv3.		   */


#include "FSMfdTypes.h"
#include "DirectOutputHelper/X52Page.h"
#include "SimClient/ReceiveBuffer.h"
#include "SimClient/FSClientTypes.h"



namespace FSMfd::Pages
{

	/// An MFD Page connected to SimConnect Simulation Variables.
	/// @remarks
	///		Handles group creation and switching during Page changes.
	/// 
	///		By the methods @a Update and @a Animate separates 
	///		concern of display-updates due to fresh game data
	///		from local updates for blinking/animation effects.
	/// 
	///		@a CleanContent is called immediately after page change,
	///		allowing to quickly present page layout before data is received.
	///		An alternative would be to set @a BackgroundReceiveEnabled,
	///		so that up-to-date data is always available.
	///		@a CleanContent's other usage is to clear obsolete data
	///		when it is stopped being received.
	/// 
	class SimPage : public DOHelper::X52Output::Page,
					public SimClient::IDataReceiver   {
	public:
		struct Dependencies;

	protected:
		using SimvarList = SimClient::SimvarList;

		SimClient::FSClient&			SimClient;
		const Duration					ContentAgeLimit;
		bool							BackgroundReceiveEnabled = false;

	private:
		SimClient::UniqueReceiveBuffer	simValues;
		SimClient::UpdateFrequency		updateFreq		 = SimClient::UpdateFrequency::PerSecond;
		TimePoint						lastUpdate		 = TimePoint::min();
		size_t							simvarCount		 = 0;
		bool							vargroupEnabled	 = false;
		bool							awaitingResponse = false;

		
		bool HasOutdatedData(TimePoint at) const;

		
		// -------- Page ------------------------------------------------

		void OnActivate(TimePoint)	 override final;
		void OnDeactivate(TimePoint) override final;


		// -------- IDataReceiver ---------------------------------------

		void Receive(SimClient::GroupId, const SimvarList&, TimePoint stamp) override;


		// -------- For descendant --------------------------------------

		/// Quickly show layout on page change, before data is available.
		virtual void CleanContent() = 0;

		/// Pull and display current data from the game.
		virtual void UpdateContent(const SimvarList&) = 0;

		/// Change state of blinking/moving parts, if any.
		virtual void StepAnimation(unsigned steps) {}

	protected:
		SimPage(uint32_t id, const Dependencies&);
		
		/// To register a SimConnect variables before the page is activated.
		void RegisterSimVar(const SimVarDef&);

		/// Set the frequency to receive updates for @a simValues.
		void SetUpdateFrequency(SimClient::UpdateFrequency);

		/// Signal that a SimConnect variable has been changed, and relevant response is imminent.
		void AwaitSimResponse()	{ awaitingResponse = true; }


		// -------- For framework ---------------------------------------
	public:
		~SimPage() override;

		bool IsAwaitingData()	   const  { return !simValues.HasData(); }
		bool IsAwaitingResponse()  const  { return awaitingResponse; }
		bool IsAwaitingReceive()   const  { return IsAwaitingData() || IsAwaitingResponse(); }
		bool HasPendingUpdate()	   const  { return lastUpdate < simValues.LastReceived(); }
		TimePoint LastUpdateTime() const  { return lastUpdate;  }

		/// Pull and display current data from game.
		/// @param stamp:	to note LastUpdateTime - avoid unnecessary clock::now() calls.
		void Update(TimePoint stamp);

		/// Change state of blinking/moving parts, if any.
		void Animate(unsigned stepCount = 1);
	};



	struct SimPage::Dependencies {
		SimClient::FSClient&	SimClient;
		const Duration			ContentAgeLimit;
	};


}	// namespace FSMfd::Pages
