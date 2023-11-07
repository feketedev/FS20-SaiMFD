#include "SimVarDef.h"



namespace FSMfd 
{

	// X52 only supports old ASCII characters, nothing fancy needed...
	static std::wstring AsDumbWString(const char* str)
	{
		size_t len = strlen(str);
		return { str, str + len };
	}


	DisplayVar::DisplayVar(std::wstring_view text, const SimVarDef& def, unsigned decimalCount) :
		definition   { def },
		text         { text },
		unitText     { AsDumbWString(def.unit) },
		decimalCount { decimalCount }
		{
	}


	DisplayVar::DisplayVar(std::wstring_view text, const SimVarDef& def,
						   std::wstring  unitText, unsigned decimalCount) :
		definition   { def },
		text         { text },
		unitText     { std::move(unitText) },
		decimalCount { decimalCount }
	{
	}


}	// namespace FSMfd