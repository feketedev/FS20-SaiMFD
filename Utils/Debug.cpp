#include "Debug.h"

#include <iostream>



namespace Debug
{
	void UseIgnorableAssertMessages()
	{
		_set_error_mode(_OUT_TO_MSGBOX);
	}


	void PrintAssertsToStderr()
	{
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW | _CRTDBG_MODE_FILE);
		_CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_WNDW | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
	}


	void Warning(const char* msg)
	{
		std::cout << "Debug warning: \t" << msg << '\n';
	}

	void Warning(const char* source, const char* msg)
	{
		std::cout << source << " warning:\t" << msg << '\n';
	}

	void Warning(const char* source, const char* msg, int param)
	{
		std::cout << source << " warning:\t" << msg << ' ' << param << '\n';
	}


	void Info(const char* msg)
	{
		std::cout << "Debug:\t\t" << msg << '\n';
	}

	void Info(const char* source, const char* msg)
	{
		std::cout << source << ":\t" << msg << '\n';
	}

	void Info(const char* source, const char* msg, int param)
	{
		std::cout << source << ":\t" << msg << ' ' << param << '\n';
	}


}	// namespace Debug
