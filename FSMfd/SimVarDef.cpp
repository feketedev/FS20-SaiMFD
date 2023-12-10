#include "SimVarDef.h"

#include "Utils/StringUtils.h"



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


	DisplayVar::DisplayVar(std::wstring text, const SimVarDef& def, unsigned short decimalCount) :
		definition   { def },
		text         { std::move(text) },
		unitText     { DefaultUnitText(def) },
		decimalCount { decimalCount }
	{
	}


	DisplayVar::DisplayVar(std::wstring text, const SimVarDef& def,
						   std::wstring unitText, unsigned short decimalCount) :
		definition   { def },
		text         { std::move(text) },
		unitText     { std::move(unitText) },
		decimalCount { decimalCount }
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
		{ "pound per hour",		L"lb" },
		{ "pounds per hour",	L"lb" },
		{ "knots",				L"kts" },
		{ "nautical mile",		L"nm" },
		{ "nautical miles",		L"nm" },
		{ "nmile",				L"nm" },
		{ "nmiles",				L"nm" },
	};


	optional<std::wstring_view>  DisplayVar::LabelWellknownUnit(const char* unit)
	{
		for (const auto& [name, label] : KnownUnitLabels)
			if (_stricmp(name, unit) == 0)
				return label;

		return Nothing;
	}


}	// namespace FSMfd