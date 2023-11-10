#include "DeviceLookup.h"

#include "FSMfdTypes.h"

#include "DirectOutputHelper/X52Output.h"
#include "DirectOutputHelper/DirectOutputInstance.h"
#include "Utils/Debug.h"
#include <iostream>



namespace FSMfd {

	using namespace DOHelper;

	
	static optional<SaiDevice>	SelectX52(DirectOutputInstance& output)
	{
		std::cout << "Found devices:";

		std::vector<SaiDevice> devices = output.EnumerateDevices();
		std::cout << (devices.empty() ? "        NONE" : "\n");

		size_t n   = 0;
		size_t sel = SIZE_MAX;
		for (const SaiDevice& dev : devices)
		{
			if (dev.type == SaiDeviceType::X52Pro)
				sel = n;

			std::cout << "\t\t" << ++n << ". " << AsString(dev.type);
		}
		std::cout << std::endl;

		if (sel > devices.size())
			return std::nullopt;

		std::cout << "\nWill use device " << (sel + 1) << '.' << std::endl;
		return devices[sel];
	}



	optional<SaiDevice>  WaitForDevice(DirectOutputInstance& directOutput, const volatile bool& proceedWait)
	{
		while (proceedWait)
		{
			optional<SaiDevice> dev = SelectX52(directOutput);
			if (dev.has_value())
				return dev;

			std::cout << "\nPlease connect X52 Pro device!" << std::endl;

			std::shared_future<SaiDevice> f2 = directOutput.WaitForNewDevice();
			while (f2.wait_for(ConnWaitTick) == std::future_status::timeout)
			{
				if (!proceedWait)
					return Nothing;
			}

			const SaiDevice& dev2 = f2.get();
			if (dev2.type == SaiDeviceType::X52Pro)
			{
				std::cout << "  Got it." << std::endl;
				return dev2;
			}
			std::cout << "  Not an X52 Pro.\n" << std::endl;
			std::this_thread::sleep_for(ConnWaitTick);
		}
		return Nothing;
	}



	optional<X52Output>  WaitForEnsuredDevice(DirectOutputInstance& directOutput, const volatile bool& proceedWait)
	{
		do
		{	
			optional<SaiDevice> selected = WaitForDevice(directOutput, proceedWait);
			if (!selected)
				return Nothing;

			X52Output x52 = directOutput.UseX52(*selected);
			if (x52.IsConnected())
				return x52;

			std::cout << "  Device unexpectedly disconnected." << std::endl;
			std::this_thread::sleep_for(ConnWaitTick);
		}
		while (proceedWait);
		
		return Nothing;
	}


}	// namespace FSMfd
