#include "ReadoutScrollList.h"
#include "Utils/StringUtils.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace FSMfd::SimClient;
	using namespace Utils::String;


#pragma region Text helpers

	static StringSection GetTargetField(std::wstring& line, const DisplayVar& var)
	{
		return { line, var.label.length(), var.ValueRoomOn(SimPage::DisplayLength) };
	}

	static std::array<StringSection, 3>	DissectLine(std::wstring& line, const DisplayVar& var)
	{
		StringSection label { line, 0, var.label.length() };
		StringSection num   = GetTargetField(line, var);
		StringSection unit  = num.FollowedBy(var.unitSymbol.length());

		return { label, num, unit };
	}


	static void PreformatReading(std::wstring& buffer, const DisplayVar& var)
	{
		auto [textTrg, _, unitTrg] = DissectLine(buffer, var);

		textTrg.FillIn(var.label);
		unitTrg.FillIn(var.unitSymbol);
	}

#pragma endregion




#pragma region Page logic

	static std::vector<std::pair<DisplayVar, SimvarPrinter>>
	ToPrintableVariables(std::vector<DisplayVar>&& dvars)
	{
		std::vector<std::pair<DisplayVar, SimvarPrinter>> res;

		for (DisplayVar& dv : dvars)
		{
			res.emplace_back(std::move(dv), CreateValuePrinterFor(dv));
		}
		return res;
	}


	ReadoutScrollList::ReadoutScrollList(uint32_t id, const Dependencies& deps, std::vector<DisplayVar> dvars) :
		SimPage   { id, deps },
		variables { ToPrintableVariables(std::move(dvars)) },
		scroller  { *this, std::max<unsigned>(3, variables.size()) }
	{
		uint32_t i = 0;
		for (const auto& [var, _] : variables)
		{
			LOGIC_ASSERT_M (var.ValueRoomOn(DisplayLength) > 0,
							"No screenspace left for value!"  );

			std::wstring text (DisplayLength, L' ');
			PreformatReading(text, var);
			scroller.SetLine(i++, std::move(text));

			RegisterSimVar(var.definition);
		}
	}


	void ReadoutScrollList::CleanContent()
	{
		uint32_t i = 0;
		for (const auto& [var, _] : variables)
		{
			StringSection field = GetTargetField(scroller.ModLine(i++), var);
			PlaceText(L"", field, Align::Left);
		}
	}


	void ReadoutScrollList::UpdateContent(const SimvarList& values)
	{
		LOGIC_ASSERT (values.VarCount() == variables.size());
		using namespace Utils::String;

		for (size_t i = 0; i < values.VarCount(); i++)
		{
			const auto& [var, print] = variables[i];

			StringSection field = GetTargetField(scroller.ModLine(i), var);

			print(values[i], field, PaddedAlignment { Align::Right, 1 });
		}
	}


	void ReadoutScrollList::OnScroll(bool up, TimePoint)
	{
		if (up)	 scroller.ScrollDown();
		else	 scroller.ScrollUp();
	}

#pragma endregion


}	// namespace FSMfd::Pages
