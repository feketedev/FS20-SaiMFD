#include "EnginesGauge.h"

#include "SimVarDef.h"
#include "DirectOutputHelper/X52Page.h"		// DisplayLen
#include "SimClient/IReceiver.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace Utils::String;


#pragma region Layouts

	constexpr unsigned DisplayLen	 = DOHelper::X52Output::Page::DisplayLength;
	constexpr unsigned DisplayCenter = DisplayLen / 2;
	

	struct EnginesGauge::Field {
		unsigned		line;
		unsigned		col;
		unsigned		len;
		PaddedAlignment	aln = { Align::Right, 0 };

		StringSection GetField(DisplayArea&) const;
	};


	/*static*/ std::vector<EnginesGauge::Field>		EnginesGauge::CreateLayout(unsigned engCount, unsigned titleLen)
	{
		LOGIC_ASSERT_M (titleLen + 4 < DisplayLen, "Gauge title too long.");

		const unsigned readoutLen = (DisplayLen - titleLen) / 2 - 1;	// for 2 values surrounding Title
		const unsigned titlePos	  = readoutLen + 1;						// prefer to keep left
		const unsigned centralPos = DisplayCenter - readoutLen / 2;

		switch (engCount)
		{
			case 1:
			case 2: 
				return {
					{ 0, titlePos,						titleLen   },
					{ 0, 0,								readoutLen, Align::Left },
					{ 0, DisplayLen - readoutLen,		readoutLen },
				};

			case 3:
				return {
					{ 1, titlePos,						titleLen   },
					{ 1, 0,								readoutLen, Align::Left },
					{ 1, DisplayLen - readoutLen,		readoutLen },
					{ 0, centralPos,					readoutLen },
				};

			default:
				return {
					{ 1, titlePos,						titleLen   },
					{ 1, 0,								readoutLen, Align::Left },
					{ 1, DisplayLen - readoutLen,		readoutLen },
					{ 0, 2,								readoutLen, Align::Left },
					{ 0, DisplayLen - readoutLen - 2,	readoutLen },
				};
		}
	}

#pragma endregion




#pragma region Construction

	static std::vector<SimVarDef> DefineVars(const SimVarDef& proto, unsigned engineCount)
	{
		std::vector<SimVarDef> defs;

		for (unsigned e = 1; e <= engineCount; e++)
		{
			defs.push_back(SimVarDef {
				proto.name + std::to_string(e),
				proto.unit,
				proto.typeReqd
			});
		}
		return defs;
	}


	EnginesGauge::EnginesGauge(unsigned engineCount, const DisplayVar& varProto) :
		StackableGauge { DisplayLen,				
						 engineCount <= 2 ? 1u : 2u,
						 DefineVars(varProto.definition, std::min(4u, engineCount)) },
		title		   { varProto.text },
		layout         { CreateLayout(engineCount, title.length()) },
		printValue	   { CreatePrinterFor(varProto.definition.typeReqd, varProto.decimalCount) }
	{
		LOGIC_ASSERT_M (varProto.definition.name.back() == ':', "EnginesGauge expects an indexable engine variable!");

		if (engineCount < 2 || 4 < engineCount)
			Debug::Warning("EngineGauge", "This gauge is intended for 2..4 engines.");
	}

#pragma endregion




#pragma region Display

	void EnginesGauge::Clean(DisplayArea& display) const
	{
		StringSection titleSect = layout[0].GetField(display);
		titleSect.FillIn(title);

		for (unsigned eng = 1; eng <= EngCount(); eng++)
		{
			StringSection s = layout[eng].GetField(display);
			s.FillWith(L' ');
		}
	}


	void EnginesGauge::Update(const SimvarSublist& measurements, DisplayArea& display) const
	{
		using namespace SimClient;

		for (unsigned eng = 1; eng <= EngCount(); eng++)
		{
			StringSection field = layout[eng].GetField(display);
			printValue(measurements[eng - 1], field, layout[eng].aln);
		}
	}


	StringSection EnginesGauge::Field::GetField(DisplayArea& fullDisplay) const
	{
		return fullDisplay[line].SubSection(col, len);
	}

#pragma endregion


}	// namespace FSMfd::Pages
