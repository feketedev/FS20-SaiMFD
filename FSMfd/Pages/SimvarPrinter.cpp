#include "SimvarPrinter.h"

#include "Utils/Debug.h"



namespace FSMfd::Pages {

	using namespace Utils::String;
	using SimClient::SimvarValue;


    SimvarPrinter CreatePrinterFor(RequestType type, unsigned decimalCount, bool truncableText)
    {
		// TODO: TypeMapping?
		switch (type)
		{
			case RequestType::Real:
				return [=](SimvarValue val, StringSection target, PaddedAlignment aln)
				{
					return PlaceNumber(val.AsDouble(), decimalCount, target, aln);
				};

			case RequestType::UnsignedInt:
				return [](SimvarValue val, StringSection target, PaddedAlignment aln)
				{
					return PlaceNumber(val.AsUnsigned32(), target, aln);
				};

			case RequestType::SignedInt:
				return [](SimvarValue val, StringSection target, PaddedAlignment aln)
				{
					DBG_BREAK_M ("NO PLACENUMBER OVERLOAD YET!");	// TODO PlaceNumber(long long)
					return PlaceNumber(val.AsInt64(), target, aln);
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


}	// namespace FSMfd::Pages
