#pragma once

#include <limits>

// Where this header is used, better hide this float macro.
#undef NAN



static_assert (std::numeric_limits<double>::has_quiet_NaN);
static_assert (std::numeric_limits<double>::has_infinity);


constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
constexpr double Inf = std::numeric_limits<double>::infinity();


inline bool IsNaN(double d)		{ return isnan(d); }
inline bool IsFinite(double d)	{ return -Inf < d && d < +Inf; }