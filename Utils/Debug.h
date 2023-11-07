#pragma once

#include <stdexcept>
#include <crtdbg.h>



#define REPORT_PRINT(lvl, ...)		_RPT_BASE(lvl, __FILE__, __LINE__, NULL, __VA_ARGS__)

#define REPORT_FAIL(x)				REPORT_PRINT(_CRT_ASSERT, #x)
#define REPORT_FAIL_M(x, msg)		REPORT_PRINT(_CRT_ASSERT, "%s\nMessage: %s", #x, msg)

#define REPORT_HRESULT(hr)			REPORT_PRINT(_CRT_ERROR, "Operation '" #hr "' failed with HRESULT: 0x%08lx", ((HRESULT)hr))
#define REPORT_HRESULT_M(hr, msg)	REPORT_PRINT(_CRT_ERROR, "Operation '" #hr "' failed with HRESULT: 0x%08lx.\nProgram message: %s", ((HRESULT)hr), msg)



#define DBG_BREAK					REPORT_PRINT(_CRT_ASSERT, "This line should not have been reached!")
#define DBG_BREAK_M(msg)			REPORT_PRINT(_CRT_ASSERT, msg)

#define DBG_ASSERT(x)				_ASSERTE(x)
#ifdef _DEBUG
#	define DBG_ASSERT_M(x, msg)		if (!(x)) { REPORT_FAIL_M(x, msg); }
#else
#	define DBG_ASSERT_M(x, msg)	
#endif



#define THROWING_ASSERT_BASE(Ex, x)			if (!(x)) { REPORT_FAIL(x);			throw Ex { #x " did not hold!" }; }
#define THROWING_ASSERT_BASE_M(Ex, x, msg)	if (!(x)) { REPORT_FAIL_M(x, msg);	throw Ex { msg }; }

// NOTE: include winerror.h to use
#define HRESULT_ASSERT_BASE(Ex, hr)			if (FAILED(hr)) { REPORT_HRESULT(hr);		throw Ex { hr }; }
#define HRESULT_ASSERT_BASE_M(Ex, hr, msg)	if (FAILED(hr)) { REPORT_HRESULT_M(x, msg);	throw Ex { hr, msg }; }



#define LOGIC_ASSERT(x)			THROWING_ASSERT_BASE   (std::logic_error, x)
#define LOGIC_ASSERT_M(x, msg)	THROWING_ASSERT_BASE_M (std::logic_error, x, msg)



// TODO: in use?
#define VERBOSE_CHK(x)			(std::cout << (x ? "OK" : "FAILED") << std::endl, (x))
#define VERBOSE_FAIL(x)			(std::cout << (x ? "FAILED" : "OK") << std::endl, !(x))



namespace Debug 
{
	// affects assert() macro only
	void UseIgnorableAssertMessages();

	// affects _CrtDbgReport -> macros in this file
	void PrintAssertsToStderr();

	void Warning(const char* msg);
	void Warning(const char* source, const char* msg);

	void Info(const char* msg);
	void Info(const char* source, const char* msg);
}
