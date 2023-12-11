#include "DirectOutputError.h"



namespace DOHelper
{

	DirectOutputError::DirectOutputError(HRESULT err, const char* msg) :
		ErrorCode { err }, std::runtime_error { msg }
	{
	}


	DirectOutputError::DirectOutputError(HRESULT err) :
		DirectOutputError { err, "DirectOutput error encountered." }
	{
	}

}
