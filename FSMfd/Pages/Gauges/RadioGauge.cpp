#include "RadioGauge.h"

#include "DirectOutputHelper/X52Page.h"		// for DisplayLength
#include "SimClient/IReceiver.h"
#include "Utils/Debug.h"
#include <charconv>


namespace FSMfd::Pages
{
	using namespace Utils::String;

	constexpr unsigned DisplayLen = DOHelper::X52Output::Page::DisplayLength;
	constexpr unsigned ValueLen   = 7;	// MMM.kkk / kkkk.dd


	static bool IsLowFreqRadio(const char* radioType)
	{
		return _strcmpi(radioType, "ADF") == 0;
	}

	
	static void PrintFreqHi(unsigned khz, StringSection trg)
	{
		wchar_t buff[ValueLen + 1];
		int written = swprintf_s(buff, L"%3u.%03u", khz / 1000, khz % 1000);
		DBG_ASSERT (written == ValueLen);

		trg.FillIn({ buff, ValueLen });  // skip \0
	}

	
	static void PrintFreqLo(unsigned hz, StringSection trg)
	{
		wchar_t buff[ValueLen + 1];
		int written = swprintf_s(buff, L"%4u.%02u", hz / 1000, hz % 1000 / 10);
		DBG_ASSERT (written == ValueLen);

		trg.FillIn({ buff, ValueLen });  // skip \0
	}

	
	static std::vector<SimVarDef>  DefineVariables(const char* radioType, uint8_t index)
	{
		LOGIC_ASSERT (index < 5);

		char iChar;
		std::to_chars(&iChar, &iChar + 1, index);

		const char* unit = IsLowFreqRadio(radioType) ? "Hz" : "KHz";

		return {
			{ std::string { radioType } + " ACTIVE FREQUENCY:"  + iChar, unit },
			{ std::string { radioType } + " STANDBY FREQUENCY:" + iChar, unit },
		};
	}



	RadioGauge::RadioGauge(const char* radioType, uint8_t index) :
		StackableGauge { DisplayLen, 1, DefineVariables(radioType, index) },
		printFreq	   { IsLowFreqRadio(radioType) ? PrintFreqLo : PrintFreqHi }
	{
	}


	void RadioGauge::Clean(DisplayArea& display)
	{
		DBG_ASSERT (display.size == 1);

		display[0].FillIn(L"  0.000    0.000");
	}


	void RadioGauge::Update(const SimvarSublist& values, DisplayArea& display)
	{
		DBG_ASSERT (display.size == 1);

		unsigned act     = values[0].AsUnsigned32();
		unsigned standby = values[1].AsUnsigned32();

		printFreq(act,	  display[0].SubSection(0, ValueLen));
		printFreq(standby, display[0].SubSection(DisplayLen - ValueLen, ValueLen));
	}


}	// namespace FSMfd::Pages

