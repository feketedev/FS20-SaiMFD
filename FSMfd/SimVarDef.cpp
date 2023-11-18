#include "SimVarDef.h"

#include "Utils/StringUtils.h"



namespace FSMfd 
{
	using Utils::String::AsDumbWString;


	DisplayVar::DisplayVar(std::wstring text, const SimVarDef& def, unsigned decimalCount) :
		definition   { def },
		text         { std::move(text) },
		unitText     { AsDumbWString(def.unit.c_str()) },
		decimalCount { decimalCount }
	{
	}


	DisplayVar::DisplayVar(std::wstring text,	  const SimVarDef& def,
						   std::wstring unitText, unsigned decimalCount) :
		definition   { def },
		text         { std::move(text) },
		unitText     { std::move(unitText) },
		decimalCount { decimalCount }
	{
	}


}	// namespace FSMfd