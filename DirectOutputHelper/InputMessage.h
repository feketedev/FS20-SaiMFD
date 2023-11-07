#pragma once

#include "DOHelperTypes.h"


namespace DOHelper 
{
	enum class MessageKind : unsigned short {

		ButtonPress,
	//	ButtonRelease		// maybe with DirectInput?

		PageActivated,
		PageDeactivated,

		COUNT
	};



	/// An asynchron input event received from Saitek DirectOutput
	struct InputMessage {
		TimePoint 		time;
		MessageKind 	kind;
		uint32_t		optData = 0;
	};


}	// namespace DOHelper
