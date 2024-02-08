#pragma once

#include "SimVarDef.h"
#include <array>



enum SIMCONNECT_DATATYPE;


namespace FSMfd::SimClient 
{
	class FSClient;

	using FSTypeMapping = std::array<SIMCONNECT_DATATYPE, AsIndex(RequestType::COUNT)>;

	using GroupId          = uint32_t;
	using VarIdx           = uint32_t;
	using NotificationCode = uint32_t;



	/// Wether ResetVarGroups will affect the Group or not.
	enum class GroupLifetime { Resettable, Permanent };

	
	/// How often SimConnect should send updated values.
	enum class UpdateFrequency { 
		PerSecond,
		FrameDriven,			// estimate 5 Hz using a fixed ratio to FPS
		OnValueChange
	};



	struct VersionNumber {
		uint32_t version[2];
		uint32_t build[2];
	};
}
