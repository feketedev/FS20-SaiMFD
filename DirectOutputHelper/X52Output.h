#pragma once

	/*  Part of FS20-SaiMFD	    Copyright 2023 Norbert Fekete  *	
	 *  Released under GPLv3.								   *
	 *	 													   *
	 *  This is a usability-wrapper around Saitek's			   *
	 *  DirectOutput library, which is Copyright 2008 Saitek.  *
	 *  See Saitek/DirectOutput.h for details.				   */


#include "InputMessage.h"
#include "ScrollwheelDebounce.h"
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <cstdint>



namespace DOHelper
{
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


	// TODO doc
	///
	class X52Output {
	public:
		class Page;
		friend class DirectOutputInstance;

		/// BiLeds and UniLeds in total
		static constexpr size_t	LedCount = 11;

	private:
		std::unique_ptr<InputQueue> 	inputQueue;
		ScrollwheelDebounce				wheelDebounce;
		std::vector<Page*>				pages;
		Page*							activePage		 = nullptr;
		std::vector<uint32_t>			actPageBeforeAdd;  // asymmetry workaround

		// Treating LED colors globally within plugin
		// - unknown+untouched LEDs cannot be queried
		std::array<optional<bool>, 19>	ledStates;				

	public:
		X52Output(void* const handle, DirectOutputInstance&);
		X52Output(X52Output&&) noexcept;
		~X52Output();

		void*					Handle()		const noexcept;
		DirectOutputInstance&	DirectOutput()  const noexcept;

		// If false, this object went stale permanently and any modifier method may throw DirectOutputError.
		bool IsConnected() const noexcept;

		/// Waits for and processes any input message within the given Duration.
		/// @remark	After the Duration control is returned to allow periodic update of data.
		void ProcessMessages(Duration);
		void ProcessMessages(TimePoint until);
		bool ProcessNextMessage(TimePoint waitUntil = TimePoint::max());
		//TODO: melyik kell vegul?


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
		void AddPage(Page&, bool activate = true);
		void AddPage(Page&, TimePoint causeStamp, bool activate = true);
		void RemovePage(Page&);
		void RemovePage(Page&, TimePoint causeStamp);


		// NOTE: Lehet ertelme?
		//bool SetProfile(const wchar_t* path);

	private:
		void Dispatch(const InputMessage&);

		Page* FindPage(uint32_t id)			 const noexcept;

		void  PageDestroys(Page&)				   noexcept;
		bool  BookRemovedPage(const Page&)		   noexcept;
		void  OrphanPageNoEx(Page&)			 const noexcept;
		void  OrphanPage(Page&)				 const;

		void  RestoreLeds();
		void  SetLedComponent(uint8_t id, bool on);
	};

}	// namespace DOHelper
