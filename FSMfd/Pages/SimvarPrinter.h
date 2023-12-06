#pragma once

#include "SimVarDef.h"
#include "SimClient/IReceiver.h"
#include "Utils/StringUtils.h"
#include <functional>



namespace FSMfd::Pages
{

	using SimvarPrinter = std::function<bool (SimClient::SimvarValue,
											  Utils::String::StringSection, 
											  Utils::String::PaddedAlignment)>;


	SimvarPrinter CreatePrinterFor(RequestType, Utils::String::DecimalLimit = 2, bool truncableText = false);

	SimvarPrinter CreateValuePrinterFor(const DisplayVar&, bool paddingCutsDecimals = true, bool truncableText = false);


}	// namespace FSMfd::Pages
