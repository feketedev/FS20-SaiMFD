#pragma once

	/*  Part of FS20-SaiMFD	    Copyright 2023 Norbert Fekete  *	
	 *  Released under GPLv3.								   *
	 *	 													   *
	 *  This is a usability-wrapper around Saitek's			   *
	 *  DirectOutput library, which is Copyright 2008 Saitek.  *
	 *  See Saitek/DirectOutput.h for details.				   */


#include "X52Output.h"


namespace DOHelper
{

	/// Buffer for the contents of single MFD page
	class X52Output::Page {
	public:
		const uint32_t 			Id;
		static constexpr size_t	DisplayLength = 16;

	private:
		X52Output* 				device		  = nullptr;
		std::wstring 			lines[3];
		bool					isDirty[3]	  = { false };

	public:
		explicit Page(uint32_t id);
		Page(Page&&) = delete;		// address is used while IsAdded

		virtual ~Page();


		/// Is added to an X52 device (either fore- or background)
		bool 					IsAdded()		const	{ return device != nullptr; }
		
		const X52Output*		CurrentDevice()	const	{ return device;   }

		/// Is the currently displayed page on CurrentDevice
		bool 					IsActive()		const	{ return IsAdded() && device->activePage == this; }
		
		/// Changed content or re-Activated since last DrawLines. (No sense while inactive.)
		bool 					IsDirty()		const	{ return isDirty[0] || isDirty[1] || isDirty[2];  }

		// i < 3
		const std::wstring& 	GetLine(char i) const	{ return lines[i]; }
		std::wstring& 			ModLine(char i);
		std::wstring&			SetLine(char i, std::wstring text, bool allowMarquee = false);

		/// Send buffered contents to X52 device.
		void DrawLines();

		/// Remove page from device. Can be re-added later.
		/// @param activateNeighbor: if IsActive, put the previous [or next as applicable] Page on screen
		///							 instead of default DirectOutput behaviour of throwing out to profile page.
		void Remove(bool activateNeighbor = false);

	private:
		friend class X52Output;

		static constexpr unsigned NotActiveError = 0xff040001;


		void Activate(TimePoint);

		/// Workaround. Try to draw a single line. Throw only for unknown error codes.
		/// @returns	true on Success, false on Failure due to Page is not being Active.
		bool ProbeActive() const;


		// ---- Event handlers for descendants ----

		virtual void OnActivate(TimePoint)			{ DrawLines(); }
		virtual void OnDeactivate(TimePoint)		{}
		virtual void OnButtonPress(TimePoint)		{}
		virtual void OnScroll(bool up, TimePoint)	{}
	};


}	// namespace DOHelper
