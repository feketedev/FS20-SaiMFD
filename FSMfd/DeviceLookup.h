#pragma once

#include "FSMfdTypes.h"



namespace FSMfd 
{
	constexpr Duration ConnWaitTick = 2s;


	optional<DOHelper::SaiDevice>	WaitForDevice(DOHelper::DirectOutputInstance&,
												  const volatile bool& proceedWait);


	optional<DOHelper::X52Output>	WaitForEnsuredDevice(DOHelper::DirectOutputInstance&,
														 const volatile bool& proceedWait);

}
