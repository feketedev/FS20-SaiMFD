#pragma once

#include "DOHelperTypes.h"
#include "Utils/LiteSharedLock.h"
#include <future>
#include <deque>
#include <guiddef.h>



namespace Saitek { class DirectOutput; }


namespace DOHelper
{

	enum class SaiDeviceType { Unknown, X52Pro, X56Stick, X56Throttle, InstrumentPanel };


	// TODO: re-const :)
	struct SaiDevice {
		void*			handle 	   	  = nullptr;
		SaiDeviceType	type       	  = SaiDeviceType::Unknown;
		wchar_t			serial[16] 	  = {0};
		GUID			directInputId = {};		// this data seems unreliable!
	};



	class DirectOutputInstance {
		friend class X52Output;


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
		const wchar_t*							pluginName;	// TODO: Capital letters!
		std::unique_ptr<Saitek::DirectOutput>	library;	// TODO: private?


		explicit DirectOutputInstance(const wchar_t* pluginName);
		DirectOutputInstance(const DirectOutputInstance&) = delete;
		~DirectOutputInstance();

		void Reset();	// ?

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
