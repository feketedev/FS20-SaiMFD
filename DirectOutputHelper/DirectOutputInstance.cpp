#include "DirectOutputInstance.h"

#include "DirectOutputError.h"
#include "X52Output.h"

#include "Saitek/DirectOutputImpl.h"
#include <cassert>
#include <shared_mutex>
#include <iostream>		//TODO



namespace DOHelper
{


#pragma region Helpers

	auto DirectOutputInstance::FindConnection(void* h)
	{
		auto hasH = [&](const HandleInUse& hin) { return hin.handle == h; };
		return std::find_if(connections.begin(), connections.end(), hasH);
	}

	auto DirectOutputInstance::FindConnection(void* h) const
	{
		auto hasH = [&](const HandleInUse& hin) { return hin.handle == h; };
		return std::find_if(connections.begin(), connections.end(), hasH);
	}



	void DirectOutputInstance::HandleInUse::MarkDisconnected()
	{
		connectionAlive.store(false, std::memory_order_release);
	}


	bool DirectOutputInstance::HandleInUse::IsAlive() const
	{
		return connectionAlive.load(std::memory_order_acquire);
	}


	// for std::vector
	auto DirectOutputInstance::HandleInUse::operator = (const HandleInUse& rhs) -> HandleInUse&
	{
		handle = rhs.handle;
		connectionAlive.store(rhs.connectionAlive.load());
		return *this;
	}

#pragma endregion



#pragma region Descriptors

	const char* AsString(SaiDeviceType t)
	{
		switch (t)
		{
			case SaiDeviceType::Unknown:			return "Unknown";
			case SaiDeviceType::X52Pro:				return "X52 Pro";
			case SaiDeviceType::X56Stick:			return "X56 Stick";
			case SaiDeviceType::X56Throttle:		return "X56 Throttle";
			case SaiDeviceType::InstrumentPanel:	return "Instrument Panel";
			default:
				throw std::logic_error("Invalid device type.");
		}
	}


	static SaiDeviceType ToSaiDeviceType(const GUID& type)
	{
		if (type == DeviceType_Fip)				return SaiDeviceType::InstrumentPanel;
		if (type == DeviceType_X52Pro)			return SaiDeviceType::X52Pro;
		if (type == DeviceType_X56_Stick)		return SaiDeviceType::X56Stick;
		if (type == DeviceType_X56_Throttle)	return SaiDeviceType::X56Throttle;

		return SaiDeviceType::Unknown;
	}


	static SaiDevice	DescribeDevice(void *handle, const Saitek::DirectOutput& library)
	{
		SaiDevice dev { handle };

		GUID typeId;
		SAI_ASSERT (library.GetDeviceType(handle, &typeId));
		dev.type = ToSaiDeviceType(typeId);

		constexpr DWORD maxLen = sizeof(dev.serial) / sizeof(wchar_t);
		HRESULT sres = library.GetSerialNumber(handle, dev.serial, maxLen);
		if (FAILED (sres))
		{
			DBG_ASSERT (sres == E_NOTIMPL);
			dev.serial[0] = 0;
		}

		SAI_ASSERT (library.GetDeviceInstance(handle, &dev.directInputId));

		return dev;
	}


	static void __stdcall	OnEnumerateDevice(void* device, void* pCtxt)
	{
		auto& list = *reinterpret_cast<std::vector<void*>*>(pCtxt);
		list.push_back(device);
	}


	std::vector<SaiDevice> DirectOutputInstance::EnumerateDevices() const
	{
		std::vector<void*> handles;
		SAI_ASSERT (library->Enumerate(&OnEnumerateDevice, &handles));


		std::vector<SaiDevice> res;
		res.reserve(handles.size());
		for (void* h : handles)
			res.push_back(DescribeDevice (h, *library));

		return res;
	}


	std::vector<SaiDevice> DirectOutputInstance::EnumerateFreeDevices() const
	{
		std::vector<void*> handles;
		SAI_ASSERT (library->Enumerate(&OnEnumerateDevice, &handles));

		std::vector<SaiDevice> res (handles.size());

		std::shared_lock readGuard { connectionListLock };

		for (void* h : handles) {
			if (FindConnection(h) == connections.end())
				res.push_back(DescribeDevice (h, *library));
		}
		return res;
	}

#pragma endregion



#pragma region Notifications

	/*static*/ void __stdcall
	DirectOutputInstance::OnDeviceChange(void* device, bool added, void* pCtxt)
	{
		std::cout << "Device " << (added ? "ADDED" : "REMOVED") << std::endl;

		auto& instance = *reinterpret_cast<DirectOutputInstance*> (pCtxt);
		if (added)
			instance.DeviceConnected(device);
		else
			instance.DeviceDisconnected(device);
	}



	// Let's try to keep promises...
	// shared_future: makes repeated calls of WaitForDevice possible
	// (This could work even if we allowed multiple DirectOutputInstances at once.)
	namespace
	{
		struct DevicePromise {
			std::promise<SaiDevice> 		promise;
			std::shared_future<SaiDevice> 	future;

			DevicePromise() : promise{}, future{ promise.get_future().share() }
			{
			}
		};
		
		static std::shared_ptr<DevicePromise>	ConnectionPromise;
	}


	std::shared_future<SaiDevice>	DirectOutputInstance::WaitForNewDevice() const
	{
		while(true)
		{
			std::shared_ptr<DevicePromise> dp = atomic_load(&ConnectionPromise);
			if (dp != nullptr)
			{
				LOGIC_ASSERT (dp->future.valid());
				return dp->future;
			}
			auto next = std::make_shared<DevicePromise>();
			if (atomic_compare_exchange_strong(&ConnectionPromise, &dp, next))
				return next->future;
		}
	}


	void DirectOutputInstance::DeviceConnected(void* h)
	{
		while (true)
		{
			std::shared_ptr<DevicePromise> dp = atomic_load(&ConnectionPromise);
			if (dp == nullptr)
				return;

			// acquire right to fulfill this promise
			std::shared_ptr<DevicePromise> n = nullptr;
			if (atomic_compare_exchange_strong(&ConnectionPromise, &dp, n))
			{
				try
				{
					LOGIC_ASSERT (dp->future.valid());
					dp->promise.set_value(DescribeDevice(h, *library));
				}
				catch (...)
				{
					dp->promise.set_exception(std::current_exception());
				}
				return;
			}
		}
	}


	// Removing MFD Pages are handled by library. We just mark the stale state.
	void DirectOutputInstance::DeviceDisconnected(void *h)
	{
		std::shared_lock readGuard { connectionListLock };

		for (HandleInUse& conn : connections)
		{
			if (conn.handle == h)
			{
				DBG_ASSERT (conn.IsAlive());
				conn.MarkDisconnected();
				break;
			}
		}
	}


	bool DirectOutputInstance::IsConnected(void* handle) const noexcept
	{
		std::shared_lock readGuard { connectionListLock };

		auto it = FindConnection(handle);
		if (it != connections.end())
			return it->IsAlive();

		return false;
	}

#pragma endregion



#pragma region Initialization

	DirectOutputInstance::DirectOutputInstance(const wchar_t *pluginName) :
		pluginName { pluginName },
		library	   { new Saitek::DirectOutput }
	{
		SAI_ASSERT (library->Initialize(pluginName));
		SAI_ASSERT (library->RegisterDeviceCallback(&OnDeviceChange, this));
	}


	DirectOutputInstance::~DirectOutputInstance()
	{
		HRESULT hr = library->Deinitialize();
		DBG_ASSERT (SUCCEEDED(hr));
	}


	void DirectOutputInstance::Reset()
	{
		std::unique_lock guard { connectionListLock };

		DBG_ASSERT_M (connections.empty(), "Called Reset while devices in use!");

		connections.clear();				// though this might handle that..
		HRESULT hr = library->Deinitialize();
		DBG_ASSERT (SUCCEEDED(hr));
		std::this_thread::yield();			// who knows, give the library time
		ConnectionPromise.reset();
		SAI_ASSERT (library->Initialize(pluginName));
	}


	X52Output	DirectOutputInstance::UseX52(const SaiDevice& device)
	{
		LOGIC_ASSERT (device.type == SaiDeviceType::X52Pro);

		X52Output x52 { device.handle, *this };
		{
			std::unique_lock guard { connectionListLock };
			connections.emplace_front(device.handle);
		}

		// Check connection - DeviceDisconnected events are live from now on.
		try
		{
			SaiDevice d = DescribeDevice(device.handle, *library);
			if (d.type == SaiDeviceType::X52Pro)
				return x52;
		}
		catch(const DirectOutputError&)
		{
		}

		// otherwise got unplugged during this request
		std::shared_lock readGuard { connectionListLock };
		auto it = FindConnection(device.handle);
		LOGIC_ASSERT(it != connections.end());
		it->MarkDisconnected();
		return x52;
	}


	void DirectOutputInstance::HandleReleased(void* h) noexcept
	{
		std::unique_lock guard { connectionListLock };

		auto it = FindConnection(h);
		if (it != connections.end())
			connections.erase(it);
		else
			DBG_BREAK;
	}

	//void DirectOutputInstance::DeviceMoved(X52Output* from, X52Output* to)
	//{
	//	auto it = std::find(inUse.begin(), inUse.end(), from);
	//	LOGIC_ASSERT (it != inUse.end());
	//	*it = to;
	//}

#pragma endregion


}	// namespace DOHelper