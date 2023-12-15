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
	constexpr unsigned DisplayCenter = DisplayLen / 2 - 1;
	constexpr unsigned TopRowInset   = 2;
	

	struct EnginesGauge::Field {
		unsigned		line;
		unsigned		col;
		unsigned		len;
		PaddedAlignment	aln = { Align::Right, 0 };

		StringSection GetField(DisplayArea&) const;
	};


	/*static*/ std::vector<EnginesGauge::Field>  EnginesGauge::CreateLayout(unsigned engCount, unsigned titleLen)
	{
		LOGIC_ASSERT_M (titleLen + 4 < DisplayLen, "Gauge title too long.");

		const unsigned readoutLen = (DisplayLen - titleLen) / 2 - 1;	// for 2 values surrounding Title
		const unsigned centralPos = (DisplayLen - readoutLen) / 2;
		const unsigned titlePos	  = readoutLen + 1;						// prefer to keep left

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
					{ 0, centralPos,					readoutLen },
					{ 1, DisplayLen - readoutLen,		readoutLen },
				};

			default:
				return {
					{ 1, titlePos,								titleLen   },
					{ 1, 0,										readoutLen, Align::Left },
					{ 0, TopRowInset,							readoutLen, { Align::Left,  1 } },
					{ 0, DisplayLen - readoutLen - TopRowInset,	readoutLen, { Align::Right, 1 } },
					{ 1, DisplayLen - readoutLen,				readoutLen },
				};
		}
	}

#pragma endregion




#pragma region Construction

	// needed by vector<Field> becuase of Field being hidden from header... well...
	EnginesGauge::~EnginesGauge()					= default;
	EnginesGauge::EnginesGauge(const EnginesGauge&)	= default;
	EnginesGauge::EnginesGauge(EnginesGauge&&)		= default;


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


	// Need to protect decimal digits, layouts utilize padding for symmetry
	static DecimalUsage OverriddenDecimalUsage(const DisplayVar& varProto)
	{
		DecimalUsage du = varProto.decimalUsage;
		du.preferDecimalsOverPadding = true;
		return du;
	}


	EnginesGauge::EnginesGauge(unsigned engineCount, const DisplayVar& varProto) :
		StackableGauge   { DisplayLen,
						   engineCount <= 2 ? 1u : 2u,
						   DefineVars(varProto.definition, std::min(4u, engineCount)) },
		title		     { varProto.label + varProto.unitSymbol },
		layout           { CreateLayout(engineCount, Practically<unsigned>(title.length())) },
		printValue	     { CreatePrinterFor(varProto.definition.typeReqd, OverriddenDecimalUsage(varProto)) }
	{
		LOGIC_ASSERT_M (varProto.definition.name.back() == ':', "EnginesGauge expects an indexable engine variable!");

		if (engineCount < 2 || 4 < engineCount)
			Debug::Warning("EngineGauge", "This gauge is intended for 2..4 engines.");
	}

#pragma endregion




#pragma region Display

	void EnginesGauge::Clean(DisplayArea& display)
	{
		StringSection titleSect = layout[0].GetField(display);
		titleSect.FillIn(title);

		for (unsigned eng = 1; eng <= EngCount(); eng++)
		{
			StringSection s = layout[eng].GetField(display);
			s.FillWith(L' ');
		}
		if (display.size > 1)
		{
			display[0].FillWith(L'¨');
			*display[0].GetStart() = L'´';
			*display[0].GetLast()  = L'`';
		}
	}


	void EnginesGauge::Update(const SimvarSublist& measurements, DisplayArea& display)
	{
		using namespace SimClient;

		for (unsigned eng = 1; eng <= EngCount(); eng++)
		{
			StringSection field = layout[eng].GetField(display);
			printValue(measurements[eng - 1], field, layout[eng].aln);
		}

		// dynamic separator dots
		if (display.size > 1)
		{
			unsigned midLen = DisplayLen - 2 * TopRowInset;
			for (wchar_t& c : display[0].SubSection(TopRowInset, midLen - 1))
				if (c == L' ')
					c = L'¨';
		}
	}


	StringSection EnginesGauge::Field::GetField(DisplayArea& fullDisplay) const
	{
		return fullDisplay[line].SubSection(col, len);
	}

#pragma endregion


}	// namespace FSMfd::Pages
