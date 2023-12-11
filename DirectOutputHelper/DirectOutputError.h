#pragma once

#include "Utils/Debug.h"
#include <winerror.h>


#define SAI_ASSERT(hr)			HRESULT_ASSERT_BASE (DirectOutputError, hr)
#define SAI_ASSERT_M(hr, msg)	HRESULT_ASSERT_BASE (DirectOutputError, hr, msg)



namespace DOHelper
{

	class  DirectOutputError : public std::runtime_error {
	public:
		const HRESULT error;

		DirectOutputError(HRESULT error, const char* msg);
		DirectOutputError(HRESULT error);
	};

}
