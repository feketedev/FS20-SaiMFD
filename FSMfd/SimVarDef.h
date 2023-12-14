#pragma once

#include "FSMfdTypes.h"
#include "Utils/BasicUtils.h"
#include "Utils/StringUtils.h"
#include <string>



namespace FSMfd
{

	enum class RequestType : unsigned short {
		UnsignedInt,
		SignedInt,
		Real,
		String,

		COUNT
	};



	/// Description of a queriable simulation variable.
	/// @remarks 
	///		Technically independent for Page configs,
	///		but definitely based on SimConnect SDK.
	struct SimVarDef {
		std::string		name;
		std::string		unit;
		RequestType		typeReqd = RequestType::UnsignedInt;

		bool operator==(const SimVarDef& rhs);
		bool operator!=(const SimVarDef& rhs)	{ return !operator==(rhs); }
	};



	/// A simulation variable with generic presentation info.
	struct DisplayVar {
		using DecimalUsage = Utils::String::DecimalUsage;
		using SignUsage	   = Utils::String::SignUsage;

		SimVarDef		definition;
		std::wstring	label;
		std::wstring	unitSymbol;
		DecimalUsage	decimalUsage;

		size_t ValueRoomOn(size_t displayLen) const
		{
			size_t occup = label.length() + unitSymbol.length();
			return Utils::SubtractTillZero(displayLen, occup);
		}

		DisplayVar(std::wstring label, SimVarDef, DecimalUsage = 2, optional<std::wstring> unitAbbrev = Nothing);
		DisplayVar(std::wstring label, SimVarDef, SignUsage,		optional<std::wstring> unitAbbrev = Nothing);
		DisplayVar(std::wstring label, SimVarDef, std::wstring unitAbbrev);

		static optional<std::wstring_view>  LabelWellknownUnit(const char* simUnit);
	};


	
	// ----------------------------------------------------------------------------------

	inline bool SimVarDef::operator==(const SimVarDef& rhs)
	{
		return name == rhs.name
			&& unit == rhs.unit
			&& typeReqd == rhs.typeReqd;
	}


}	// namespace FSMfd
