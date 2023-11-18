#pragma once

#include "SimVarDef.h"
#include "SimClient/IReceiver.h"
#include "Utils/StringUtils.h"
#include <functional>



namespace FSMfd::Pages {

	using SimvarPrinter = std::function<bool (SimClient::SimvarValue,
											  Utils::String::StringSection, 
											  Utils::String::PaddedAlignment)>;


	SimvarPrinter CreatePrinterFor(RequestType, unsigned decimalCount = 2, bool truncableText = false);


}	// namespace FSMfd::Pages
