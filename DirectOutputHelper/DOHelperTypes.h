#pragma once

#include <chrono>
#include <cstdint>
#include <optional>



namespace DOHelper
{
	using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
	using Duration 	= TimePoint::duration;

	static_assert(TimePoint::clock::is_steady, "Rather check steady_clock precision...");


	using std::optional;
	constexpr std::nullopt_t	Nothing = std::nullopt;


	class DirectOutputInstance;
	class X52Output;
	class InputQueue;
	struct SaiDevice;
	
	enum class SaiDeviceType;
	
	enum class LedColor : uint8_t;
	
	// these have disjoint values
	enum BiLed  : uint32_t;
	enum UniLed : uint32_t;
}