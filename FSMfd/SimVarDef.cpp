#include "SimVarDef.h"



namespace FSMfd 
{
	using Utils::String::AsDumbWString;


	static std::wstring DefaultUnitText(const SimVarDef& def)
	{
		const char* name = def.unit.c_str();

		optional<std::wstring_view> known = DisplayVar::LabelWellknownUnit(name);
		if (known.has_value())
			return std::wstring { *known };

		return AsDumbWString(name);
	}


	DisplayVar::DisplayVar(std::wstring label, SimVarDef def, DecimalUsage decimalUsage, optional<std::wstring> unitAbbrev) :
		definition   { std::move(def) },
		label        { std::move(label) },
		unitSymbol   { unitAbbrev.has_value() ? *std::move(unitAbbrev) : DefaultUnitText(definition)},
		decimalUsage { decimalUsage }
	{
	}


	DisplayVar::DisplayVar(std::wstring label, SimVarDef def, SignUsage signUsage, optional<std::wstring> unitAbbrev) :
		DisplayVar { std::move(label), std::move(def), DecimalUsage { 2, signUsage }, std::move(unitAbbrev) }
	{
	}


	DisplayVar::DisplayVar(std::wstring label, SimVarDef def, std::wstring unitAbbrev) :
		DisplayVar { std::move(label), std::move(def), DecimalUsage { 2 }, std::move(unitAbbrev) }
	{
	}



	const std::pair<const char*, const std::wstring_view> KnownUnitLabels[] = {
		{ "rpm",				L""	},
		{ "bool",				L""	},
		{ "number",				L""	},
		{ "ratio",				L""	},
		{ "percent",			L"%" },
		{ "celsius",			L"°C" },
		{ "kelvin",				L"°K" },
		{ "farenheit",			L"°F" },
		{ "fahrenheit",			L"°F" },
		{ "rankine",			L"°R" },
		{ "ratian",				L"rad" },
		{ "degree",				L"°" },
		{ "degrees",			L"°" },
		{ "pascal",				L"Pa" },
		{ "pascals",			L"Pa" },
		{ "kilopascal",			L"kPa" },
		{ "pound",				L"lb" },
		{ "pounds",				L"lb" },
		{ "lbs",				L"lb" },	// just save space
		{ "foot pounds",		L"lbf" },
		{ "pound per hour",		L"pph" },
		{ "pounds per hour",	L"pph" },
		{ "gallon per hour",	L"gph" },
		{ "gallons per hour",	L"gph" },
		{ "knots",				L"kts" },
		{ "nautical mile",		L"nm" },
		{ "nautical miles",		L"nm" },
		{ "nmile",				L"nm" },
		{ "nmiles",				L"nm" },
		{ "decibel",			L"dB" },
		{ "decibels",			L"dB" },
	};


	optional<std::wstring_view>  DisplayVar::LabelWellknownUnit(const char* unit)
	{
		for (const auto& [name, label] : KnownUnitLabels)
			if (_stricmp(name, unit) == 0)
				return label;

		return Nothing;
	}


}	// namespace FSMfd