#pragma once

#include "FSClientTypes.h"



namespace FSMfd::SimClient
{
	// TODO: Configurator.h?
	FSTypeMapping	GetDefaultTypeMapping();
	
	size_t			GetSizeOf(SIMCONNECT_DATATYPE);

}

