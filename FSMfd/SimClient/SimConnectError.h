#pragma once

#include "Utils/Debug.h"
#include <winerror.h>


#define FS_ASSERT(hr)			HRESULT_ASSERT_BASE (FSMfd::SimClient::SimConnectError, hr)
#define FS_ASSERT_M(hr, msg)	HRESULT_ASSERT_BASE (FSMfd::SimClient::SimConnectError, hr, msg)



namespace FSMfd::SimClient
{

	class SimConnectError : public std::runtime_error
	{
	public:
		const HRESULT error;

		SimConnectError(HRESULT error, const char* msg);
		SimConnectError(HRESULT error);
	};

}

