#pragma once

#include "X52Output.h"


namespace DOHelper
{

	/// Buffer for the contents of single MFD page
	class X52Output::Page {
	public:
		const uint32_t 			id;

		static constexpr size_t	DisplayLength = 16;

	private:
		X52Output* 				device		  = nullptr;
		std::wstring 			lines[3];
		bool					isDirty[3]	  = { false };

	public:
		explicit Page(uint32_t id);
		Page(Page&&) = delete;				// address is used while IsAdded

		virtual ~Page();


		/// Is added to an X52 device (either fore- or background)
		bool 					IsAdded()		const	{ return device != nullptr; }
		
		const X52Output*		CurrentDevice()	const	{ return device;   }

		/// Is the currently displayed page on CurrentDevice
		bool 					IsActive()		const	{ return IsAdded() && device->activePage == this; }

		// i < 3
		const std::wstring& 	GetLine(char i) const	{ return lines[i]; }
		std::wstring& 			ModLine(char i);
		std::wstring&			SetLine(char i, std::wstring text, bool allowMarquee = false);

		/// Send buffered contents to X52 device
        /// @param forceAll: send all lines, not only the changed subset
		void DrawLines(bool forceAll = false) const;

		//void AddTo(X52Output&, bool activate = true);
		//void Remove();

	private:
		friend class X52Output;

		void Activate(TimePoint);

		virtual void OnActivate(TimePoint)			{ DrawLines(); }
		virtual void OnDeactivate(TimePoint)		{}
		virtual void OnButtonPress(TimePoint)		{}
		virtual void OnScroll(bool up, TimePoint)	{}
	};

}	// namespace DOHelper