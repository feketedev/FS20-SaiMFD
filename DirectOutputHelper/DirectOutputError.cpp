#include "DirectOutputError.h"



namespace DOHelper
{

	DirectOutputError::DirectOutputError(HRESULT err, const char* msg) :
		error { err }, std::runtime_error { msg }
	{
	}


	DirectOutputError::DirectOutputError(HRESULT err) :
		DirectOutputError { err, "DirectOutput error encountered." }
	{
	}

}
