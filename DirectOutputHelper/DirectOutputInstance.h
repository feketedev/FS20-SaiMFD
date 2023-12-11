#pragma once

	/*  Part of FS20-SaiMFD			  Copyright 2023 Norbert Fekete  *	
	 *  Released under GPLv3.										 *
	 *  Consult LICENSE.txt or https://www.gnu.org/licenses/gpl-3.0  *
	 *	 															 *
	 *  This module is a usability-wrapper around Saitek's			 *
	 *  DirectOutput library, which is Copyright 2008 Saitek.		 *
	 *  See Saitek/DirectOutput.h for details.						 */


#include "DOHelperTypes.h"
#include "Utils/LiteSharedLock.h"
#include <array>
#include <future>
#include <deque>
#include <guiddef.h>



namespace Saitek { class DirectOutput; }


namespace DOHelper
{

	enum class SaiDeviceType { Unknown, X52Pro, X56Stick, X56Throttle, InstrumentPanel };


	struct SaiDevice {
		void* const						Handle;
		const SaiDeviceType				Type;
		const std::array<wchar_t, 16>	Serial;
		const GUID						DirectInputId;	// this data seems unreliable!
	};



	class DirectOutputInstance {
		friend class X52Output;
		friend class InputQueue;

		std::unique_ptr<Saitek::DirectOutput>	library;

		struct HandleInUse {
			void*				handle;
			std::atomic_bool	connectionAlive = true;

			void MarkDisconnected();
			bool IsAlive()	const;
			HandleInUse(void* h) : handle { h } {}
			HandleInUse& operator = (const HandleInUse& rhs);
		};
		std::deque<HandleInUse>			connections;
		mutable Utils::LiteSharedLock	connectionListLock;

	public:
		const wchar_t* const			PluginName;


		explicit DirectOutputInstance(const wchar_t* pluginName);
		DirectOutputInstance(const DirectOutputInstance&) = delete;
		~DirectOutputInstance();

		void Reset();

		std::vector<SaiDevice>			EnumerateDevices()		const;
		std::vector<SaiDevice>			EnumerateFreeDevices()	const;
		std::shared_future<SaiDevice>	WaitForNewDevice() 		const;

		X52Output UseX52(const SaiDevice&);

	private:
		auto FindConnection(void* handle);
		auto FindConnection(void* handle) const;
		bool IsConnected(void* handle)	  const noexcept;
		void HandleReleased(void*)		  noexcept;

		void 					DeviceDisconnected(void*);
		void 					DeviceConnected(void*);
		static void __stdcall 	OnDeviceChange(void* device, bool added, void* pCtxt);
	};


	const char*	AsString(SaiDeviceType t);


}	// namespace DOHelper
