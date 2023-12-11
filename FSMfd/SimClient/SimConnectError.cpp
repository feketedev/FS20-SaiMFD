#include "SimConnectError.h"



namespace FSMfd::SimClient
{

	SimConnectError::SimConnectError(HRESULT err, const char* msg) :
		ErrorCode { err }, std::runtime_error { msg }
	{
	}

	SimConnectError::SimConnectError(HRESULT err) :
		SimConnectError { err, "SimConnect error encountered." }
	{
	}

}
