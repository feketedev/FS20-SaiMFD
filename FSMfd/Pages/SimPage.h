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
	// TODO finish doc
	/// An MFD Page connected to SimConnect Simulation Variables.
	/// @remarks
	///		Handles group creation and switching during Page changes.
	/// 
	///		Via the methots @m Update and @m Animate separates 
	///		concern of display-updates due to fresh game data
	///		and local updates for blinking/animation effects.
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
		std::vector<SimVarDef>			simDefinitions;
		SimClient::UniqueReceiveBuffer	simValues;
		TimePoint						lastUpdate;
		bool							vargroupEnabled	   = false;
		bool							awaitingResponse   = false;

		
		// -------- Page ------------------------------------------------

		void OnActivate(TimePoint)	 override final;
		void OnDeactivate(TimePoint) override final;


		// -------- IDataReceiver ---------------------------------------

		void Receive(SimClient::GroupId, const SimvarList&, TimePoint stamp) override;


		// -------- For descendant --------------------------------------

		/// Quickly show layout on page change, before data is available.
		virtual void CleanContent()  = 0;

		/// Pull and display current data from the game.
		virtual void UpdateContent(const SimvarList&) = 0;

		/// Change state of blinking/moving parts, if any.
		virtual void StepAnimate(unsigned steps) {}


		// -------- Private helpers -------------------------------------

		bool HasOutdatedData(TimePoint at) const;

	protected:
		SimPage(uint32_t id, const Dependencies&);
		
		void RegisterSimVar(const SimVarDef&);

		void AwaitSimResponse()	{ awaitingResponse = true; }

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