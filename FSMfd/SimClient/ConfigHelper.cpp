#include "ConfigHelper.h"

#include "Utils/BasicUtils.h"
#include "Utils/Debug.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SimConnect.h"



namespace FSMfd::SimClient
{

	using Utils::AsIndex;


	FSTypeMapping GetDefaultTypeMapping()
	{
		FSTypeMapping map;
		map[AsIndex(RequestType::Real)]        = SIMCONNECT_DATATYPE_FLOAT64;
		map[AsIndex(RequestType::UnsignedInt)] = SIMCONNECT_DATATYPE_INT32;
		map[AsIndex(RequestType::SignedInt)]   = SIMCONNECT_DATATYPE_INT64;
		map[AsIndex(RequestType::String)]      = SIMCONNECT_DATATYPE_STRING256;

		return map;
	}


	size_t GetSizeOf(SIMCONNECT_DATATYPE typ)
	{
		switch (typ)
		{
			case SIMCONNECT_DATATYPE_INT32:
			case SIMCONNECT_DATATYPE_FLOAT32:
				return 4;

			case SIMCONNECT_DATATYPE_INT64:
			case SIMCONNECT_DATATYPE_FLOAT64:
			case SIMCONNECT_DATATYPE_STRING8:
				return 8;

			case SIMCONNECT_DATATYPE_STRING32:
				return 32;

			case SIMCONNECT_DATATYPE_STRING64:
				return 64;

			case SIMCONNECT_DATATYPE_STRING128:
				return 128;

			case SIMCONNECT_DATATYPE_STRING256:
				return 256;

			case SIMCONNECT_DATATYPE_STRING260:
				return 260;

			case SIMCONNECT_DATATYPE_STRINGV:
				DBG_BREAK_M ("Type support not implemented.");
				throw std::logic_error("Type support not implemented.");

			default:
				DBG_BREAK_M ("Unsupported type for query!");
				throw std::logic_error("Unsupported type for query!");
		}
	}


}	// namespace FSMfd::SimClient
