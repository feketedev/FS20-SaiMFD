
	/*  FS20-SaiMFD					  Copyright 2023 Norbert Fekete  *
	 *  Released under GPLv3.										 *
	 *  Consult LICENSE.txt or https://www.gnu.org/licenses/gpl-3.0  */


#include "DeviceLookup.h"
#include "MfdLoop.h"

#include "DirectOutputHelper/DirectOutputInstance.h"
#include "DirectOutputHelper/DirectOutputError.h"
#include "DirectOutputHelper/X52Output.h"
#include "SimClient/ConfigHelper.h"
#include "Utils/Debug.h"
#include "Utils/IoUtils.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>



namespace FSMfd {

	constexpr char	  FSClientName[]  = "FS20-SaiMFD";
	constexpr wchar_t SaiPluginName[] = L"FS2020 MFD";


	volatile bool Uninterrupted = true;


	static BOOL		WINAPI ConsoleInterrupt(DWORD CtrlType)
	{
		Uninterrupted = false;
		std::cout << "\nExit requested...\n" << std::endl;
		return TRUE;
	}


	static void		Run(DOHelper::DirectOutputInstance& directOutput)
	{
		const SimClient::FSTypeMapping typeMapping = SimClient::GetDefaultTypeMapping();

		optional<DOHelper::X52Output> x52;
		try
		{
			optional<DOHelper::X52Output> found = WaitForEnsuredDevice(directOutput, Uninterrupted);
			if (!found)
			{
				DBG_ASSERT (!Uninterrupted);	// user quit
				return;
			}
			x52.emplace(*std::move(found));

			MfdLoop loop { Uninterrupted, FSClientName, typeMapping };
			loop.Run(*x52);
		}
		catch (const DOHelper::DirectOutputError& err)
		{
			Utils::FormatFlagScope guard { std::cerr };

			// check if Disconnection got noticed
			if (x52.has_value() && !x52->IsConnected())
				std::cerr << "\nConnection with device interrupted!\n" << std::endl;
			else
				std::cerr << err.what()
						  << "\n(code: 0x" << std::hex << err.error << ')' << std::endl;
		}
		catch (const std::logic_error& ex)
		{
			std::cerr << "\nInternal error : " << ex.what() << std::endl;
		}
	}

}	// namespace FSMfd




int main()
{
	using namespace std::chrono_literals;

	std::cout << "  *-------------------------------------------------------------------*\n"
				 "  | FS2020 <--> Saitek X52 MFD   Copyright 2023 Norbert Fekete        |\n"
				 "  | Utilizing DirectOutput SDK   Copyright 2008 Saitek                |\n"
				 "  | Released under GPLv3. See:   https://www.gnu.org/licenses/gpl-3.0 |\n"
				 "  *-------------------------------------------------------------------*\n"
			  << std::endl;

	SetConsoleCtrlHandler(&FSMfd::ConsoleInterrupt, TRUE);
	Debug::PrintAssertsToStderr();

	try
	{
		std::cout << "Init DirectOutput...    ";
		DOHelper::DirectOutputInstance output { FSMfd::SaiPluginName };
		std::cout << "OK" << std::endl;

		while (true)
		{
			FSMfd::Run(output);
			
			// user quit
			if (!FSMfd::Uninterrupted)
				return 0;

			// cought recoverable error
			std::this_thread::sleep_for(1s);
			std::cout << "Reconnecting..." << std::endl;
			output.Reset();
		}
	}
	catch (const DOHelper::DirectOutputError& err)
	{
		// DirectOutputInstance initialization failure
		std::cerr << "FAILED\n" << err.what()
				  << "\n  (code: 0x" << std::hex << err.error << ')'
				  << "\nWill now quit." << std::endl;

		std::cin.ignore();
		return 5;
	}
	catch (const std::exception& ex)
	{
		std::cerr << "\nUnexpected error!\n" << ex.what() 
				  << "\nWill now quit." << std::endl;

		std::cin.ignore();
		return 1;
	}
}

