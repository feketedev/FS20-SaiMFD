#pragma once

#include "Utils/BasicUtils.h"
#include <string>



namespace FSMfd {

	enum class RequestType : unsigned short {
		UnsignedInt,
		SignedInt,
		Real,
		String,

		COUNT	// TODO: convention?
	};



	/// Description of a queriable simulation variable.
	/// @remarks 
	///		Technically independent for Page configs,
	///		but definitely based on SimConnect SDK.
	struct SimVarDef {
		const char*		name;
		const char*		unit;
		RequestType		typeReqd = RequestType::UnsignedInt;

		bool operator== (const SimVarDef& rhs);
		bool operator!= (const SimVarDef& rhs)	{ return !operator==(rhs); }
	};



	/// A simulation variable with generic presentation info.
	struct DisplayVar {
		SimVarDef				 definition;
		const std::wstring_view	 text;
		const std::wstring		 unitText;
		unsigned				 decimalCount = 2;		// discarded for ints

		size_t ValueRoomOn(size_t displayLen) const
		{
			size_t occup = text.length() + unitText.length();
			return Utils::SubtractTillZero(displayLen, occup);
		}

		DisplayVar(std::wstring_view text, const SimVarDef&, unsigned decimalCount = 2);
		DisplayVar(std::wstring_view text, const SimVarDef&,
				   std::wstring		 unitTextOverride,		 unsigned decimalCount = 2);
	};


	
	// ----------------------------------------------------------------------------------

	inline bool SimVarDef::operator== (const SimVarDef& rhs)
	{
		return (name == rhs.name || strcmp(name, rhs.name) == 0)
			&& (unit == rhs.unit || strcmp(unit, rhs.unit) == 0)
			&& typeReqd == rhs.typeReqd;
	}


}	// namespace FSMfd
