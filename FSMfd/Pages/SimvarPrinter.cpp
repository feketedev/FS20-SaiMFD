#include "SimvarPrinter.h"

#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace Utils::String;
	using SimClient::SimvarValue;


    SimvarPrinter CreatePrinterFor(RequestType type, DecimalUsage options, bool truncableText)
    {
		// MAYBE: Consider SimClient::FSTypeMapping? Currently a single signed/unsigned/fp. type can be chosen.
		//		  FSMfd is fixed to use 32 bit integers and double.
		//		  For strings, width can be configured freely - reading their value is independent of buffer length.
		switch (type)
		{
			case RequestType::Real:
				return [=](SimvarValue val, StringSection target, PaddedAlignment aln)
				{
					return PlaceNumber(val.AsDouble(), options, target, aln);
				};

			case RequestType::UnsignedInt:
				return [](SimvarValue val, StringSection target, PaddedAlignment aln)
				{
					return PlaceNumber(val.AsUnsigned32(), target, aln);
				};

			case RequestType::SignedInt:
				return [sign{options.sign}](SimvarValue val, StringSection target, PaddedAlignment aln)
				{
					return PlaceNumber(val.AsInt32(), sign, target, aln);
				};

			case RequestType::String:
				// TODO proper string->wstring conversion for code-pages?
				return truncableText 
					?	[](SimvarValue val, StringSection target, PaddedAlignment aln)
						{
							std::wstring ws = AsDumbWString(val.AsString());
							size_t wrote = PlaceTruncableText(ws, target, aln);
							return wrote > 0;
						}
					:	[](SimvarValue val, StringSection target, PaddedAlignment aln)
						{
							std::wstring ws = AsDumbWString(val.AsString());
							return PlaceText(ws, target, aln);
						};

			default:
				DBG_BREAK;
				throw std::logic_error("Unknown type requested to print.");
		}
	}


	SimvarPrinter CreateValuePrinterFor(const DisplayVar& dv, bool truncable)
	{
		return CreatePrinterFor(dv.definition.typeReqd, dv.decimalUsage, truncable);
	}


}	// namespace FSMfd::Pages
