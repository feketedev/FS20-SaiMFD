#pragma once

	/*  Part of FS20-SaiMFD	    Copyright 2023 Norbert Fekete  *	
	 *  Released under GPLv3.								   *
	 *	 													   *
	 *  This is a usability-wrapper around Saitek's			   *
	 *  DirectOutput library, which is Copyright 2008 Saitek.  *
	 *  See Saitek/DirectOutput.h for details.				   */


#include "ScrollwheelDebounce.h"
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <cstdint>



namespace DOHelper
{
	struct InputMessage;


	/// Possible colors of a BiLed.
	enum class LedColor : uint8_t { Off, Green, Red, Amber };


	/// Bi-color LEDs on X52 Pro
	enum BiLed : uint32_t {
		ButtonA	= 1,
		ButtonB	= 3,
		ButtonD	= 5,
		ButtonE	= 7,
		ButtonI	= 17,
		Pov2	= 15,
		Toggle1	= 9,
		Toggle2	= 11,
		Toggle3 = 13
	};


	/// Simple on-off LEDs on X52 Pro.
	enum UniLed : uint32_t {
		Throttle = 19,
		Fire	 = 0
	};


	/// DirectOutput wrapper for the the functions of an X52 device.
	/// @remarks
	///	  *	Synchronizes bare DirectOutput events firing on various threads
	///   *	Keeps track of the active page
	///	  *	Provides complete page activation/deactivation events
	///	  *	Attempts to debounce scrollwheel inputs (the API behaves suspiciously wild for me...)
	///   *	Instead of DirectOutput's per-page LED handling, stores a global state of LEDs, which
	///		it automatically reapplies after any page change (within the plugin)
	class X52Output {
	public:
		class Page;
		friend class DirectOutputInstance;

		/// BiLeds and UniLeds in total
		static constexpr uint32_t	LedCount = 11;
		static constexpr uint32_t	LedIdMax = 19;

	private:
		std::unique_ptr<InputQueue> 	inputQueue;
		ScrollwheelDebounce				wheelDebounce;
		std::vector<Page*>				pages;
		Page*							activePage				= nullptr;
		bool							activePageLagsBehind	= false;	// processing lags behind DirectOutput state
		optional<uint32_t>				expectedPageDeactivation;			// message reorder workaround

		// Treating LED colors globally within plugin
		// - unknown+untouched LEDs cannot be queried
		// - this is targeted plugin state, set before commanding the device!
		std::array<optional<bool>, 20>	ledStates;

	public:
		X52Output(void* handle, DirectOutputInstance&);
		X52Output(X52Output&&) noexcept;
		~X52Output();

		void*					Handle()		const noexcept;
		DirectOutputInstance&	DirectOutput()  const noexcept;

		// If false, this object went stale permanently and any modifier method may throw DirectOutputError.
		bool IsConnected() const noexcept;

		/// Waits for and processes any input message until the given TimePoint.
		/// @remark 	After the TimePoint control is returned to allow periodic update of data.
		void ProcessMessages(TimePoint until);

		/// Waits for the next input message until the given TimePoint and processes it if arrives.
		/// @returns	Received anything (not timed out). 
		bool ProcessNextMessage(TimePoint waitUntil = TimePoint::max());


		// ---- LED functions --------------------

		// Not feasible via the API
		//LedColor 	GetColor(BiLed) 	const;
		//bool 		GetState(UniLed)	const;
	
		void 		SetColor(BiLed, LedColor);
		void 		SetState(UniLed, bool on);


		// ---- MFD page functions ---------------

		Page* 		GetActivePage()			{ return activePage; }
		const Page*	GetActivePage() const	{ return activePage; }

		bool HasPage(uint32_t id)	const;
		bool HasPages()				const;
	
		void ClearPages();
		void AddPage(Page&,						  bool activate = true);
		void AddPage(Page&, TimePoint causeStamp, bool activate = true);
		void RemovePage(Page&,						 bool activateNeighbor = false);
		void RemovePage(Page&, TimePoint causeStamp, bool activateNeighbor = false);


		// NOTE: Would it make sense to utilize this capability?
		//bool SetProfile(const wchar_t* path);

	private:
		void  Dispatch(const InputMessage&);
		bool  TryActivatePage(Page*, TimePoint);
		Page* DeactivateCurrentPage(TimePoint stamp);

		Page* FindPage(uint32_t id)			 const noexcept;

		void  PageDestroys(Page&)				   noexcept;
		bool  BookRemovedPage(const Page&)		   noexcept;
		void  OrphanPageNoEx(Page&)			 const noexcept;
		void  OrphanPage(Page&)				 const;

		void  RestoreLeds();
		void  SetLedComponent(uint8_t id, bool on);

		bool  RecoverActivePage(TimePoint);
		Page* FindActivePageByProbing() const;
		
		template<class SaiActionPtr, class... Args>
		bool  TryWithActivePage(const SaiActionPtr&, const Args&...);

		// for Page (hide types instead of explicit instantiation)
		bool  TrySetActivePageLine(uint32_t i, uint32_t len, const wchar_t* text);
	};


}	// namespace DOHelper
