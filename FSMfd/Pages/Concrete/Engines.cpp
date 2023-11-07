#include "Engines.h"
#include "Utils/StringUtils.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace Utils::String;

	constexpr unsigned EngCount = 2;

	const unsigned Positions[] = {0, 12};


	// TODO
	//template<size_t Pos, size_t MaxLen, size_t Decimals>
	//static void PlaceNumber(std::wstring& buffer, int32_t value)
	//{
	//	wchar_t* trg = buffer.data() + pos;

	//	wchar_t num[12];		// for a 32 bit signed
	//	int numLen = swprintf_s(num, L"%*d ", 3, value);

	//	//if (numLen <= 0)
	//	//	numLen = swprintf_s(num, L"%*d", 3, value);

	//	//ASSUME (numLen == maxDigits);		// padding included

	//	if (0 < numLen && numLen <= maxLen)
	//	{
	//		for (size_t i = 0; i < numLen; i++)
	//			trg[i] = num[i];
	//		return;
	//	}

	//	// pattern for too large number or unknown error - 1 char space guaranteed
	//	size_t s = maxLen - std::min<size_t>(3, maxLen);
	//	for (size_t i = s; i < maxLen; i++)
	//		trg[i] = L'#';
	//}


	void Engines::CleanContent()
	{
		for (size_t i = 1; i < scroller.LineCount(); i++)
		{
			scroller.ModLine(i)
					.replace(0, 5, 5, L' ')
					.replace(9, 5, 5, L' ');
		}
	}


	void Engines::UpdateContent(const SimvarList&)
	{
		const std::vector<int32_t> values = GetSimVars();

		ASSUME (values.size() == simvars.size() * EngCount);

		for (size_t i = 0; i < simvars.size(); i++)
		{
			for (unsigned e = 1; e <= EngCount; e++)
			{
				// TODO: cont. impl.!
				const int32_t& val = values[i * EngCount + e - 1];
				//PlaceNumber(scroller.ModLine(i + 1),
				//			Positions[e - 1], 4,		val);
			}
		}
	}

	void Engines::OnScroll(bool up)
	{
		if (up)	 scroller.ScrollDown();
		else	 scroller.ScrollUp();
	}


	// Extract what's needed for SimPage
	static std::vector<SimVarDef>	SimVarDefsFrom(const std::vector<Engines::DisplayVar>& vars)
	{
		std::vector<SimVarDef> defs;
		defs.reserve(vars.size());

		uint32_t idx = 0;
		for (const auto& v : vars)
		{
			for (unsigned e = 1; e <= EngCount; e++)
			{
				char num = '0' + e;

				std::string name { v.name };
				name.append(1, num);

				defs.emplace_back(idx++, name.data(), v.unit);
			}
		}
		return defs;
	}



	Engines::Engines(uint32_t id, FSClient& client, std::vector<DisplayVar> varDefs) :
		SimPage  { id, client, SimVarDefsFrom(simvars) },
		simvars { std::move(varDefs) },
		scroller { *this, simvars.size() + 1, false }
	{
		//DefineSimVars(SimVarDefsFrom(simvars));

		scroller.SetLine(0, L"  1         2   ");

		uint32_t i = 1;
		for (const DisplayVar& var : simvars)
		{
			scroller.SetLine(i++, AlignCenter(DisplayLength, var.text));
		}
	}




}	// namespace FSMfd::Pages