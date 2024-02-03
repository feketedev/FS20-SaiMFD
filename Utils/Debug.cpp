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



	bool EnableVerboseInfo = false;



	static const char* CurrentInlineSource = nullptr;

	/// Ensure separate line for message if previous wasn't Inline from the same source.
	/// @returns:	at start of new line.
	static bool CleanLine(const char* forSource = nullptr)
	{
		bool change = CurrentInlineSource != forSource;
		if (change && CurrentInlineSource != nullptr)
			std::cout << '\n';
			
		CurrentInlineSource = forSource;
		return change;
	}


	void Warning(const char* msg)
	{
		CleanLine();
		std::cout << "Debug warning: \t" << msg << '\n';
	}

	void Warning(const char* source, const char* msg)
	{
		CleanLine();
		std::cout << source << " warning:\t" << msg << '\n';
	}

	void Warning(const char* source, const char* msg, int param)
	{
		CleanLine();
		std::cout << source << " warning:\t" << msg << ' ' << param << '\n';
	}


	void Info(const char* msg)
	{
		if (!EnableVerboseInfo)
			return;

		CleanLine();
		std::cout << "Debug:\t\t" << msg << '\n';
	}

	void Info(const char* source, const char* msg)
	{
		if (!EnableVerboseInfo)
			return;

		CleanLine();
		std::cout << source << ":\t" << msg << '\n';
	}

	void Info(const char* source, const char* msg, int param)
	{
		if (!EnableVerboseInfo)
			return;

		CleanLine();
		std::cout << source << ":\t" << msg << ' ' << param << '\n';
	}

	void InlineInfo(const char* source, const char* msg)
	{
		if (!EnableVerboseInfo)
			return;

		if (CleanLine(source))
			std::cout << source << ":\t" << msg;
		else
			std::cout << msg;	
	}

	void InlineInfo(const char* source, const char* msg, int param)
	{
		if (!EnableVerboseInfo)
			return;

		InlineInfo(source, msg);
		std::cout << ' ' << param;
	}


}	// namespace Debug
