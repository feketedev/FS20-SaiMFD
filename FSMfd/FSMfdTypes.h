#pragma once

#include "DirectOutputHelper/DOHelperTypes.h"
#include <optional>



namespace FSMfd
{
	using namespace std::chrono_literals;

	using TimePoint = DOHelper::TimePoint;
	using Duration = TimePoint::duration;

	using std::pair;
	using std::optional;
	constexpr std::nullopt_t	Nothing = std::nullopt;

	class  Configurator;
	struct SimVarDef;


	namespace SimClient 
	{
		using GroupId          = uint32_t;
		using VarIdx           = uint32_t;
		using NotificationCode = uint32_t;

		class  DedupSimvarRegister;
		class  SimvarList;
		struct SimvarValue;
	}


	namespace Led
	{
		using DOHelper::BiLed;
		using DOHelper::UniLed;

		using Color = DOHelper::LedColor;

		class IStateDetector;
		class LedControl;
	}
}
