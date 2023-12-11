#pragma once

#include "FSMfdTypes.h"
#include "Utils/BasicUtils.h"
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
		SimVarDef		definition;
		std::wstring	text;
		std::wstring	unitText;
		unsigned short	decimalCount = 2;		// discarded for ints

		size_t ValueRoomOn(size_t displayLen) const
		{
			size_t occup = text.length() + unitText.length();
			return Utils::SubtractTillZero(displayLen, occup);
		}

		DisplayVar(std::wstring text, const SimVarDef&, unsigned short decimalCount = 2);
		DisplayVar(std::wstring text, const SimVarDef&,
				   std::wstring unitTextOverride,		unsigned short decimalCount = 2);

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
