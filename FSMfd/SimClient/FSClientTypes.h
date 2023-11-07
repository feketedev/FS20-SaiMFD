#pragma once

#include "SimVarDef.h"
#include <array>



enum SIMCONNECT_DATATYPE;


namespace FSMfd::SimClient 
{
	class FSClient;

	using FSTypeMapping = std::array< SIMCONNECT_DATATYPE, 
									  static_cast<size_t>(RequestType::COUNT) >;
}
