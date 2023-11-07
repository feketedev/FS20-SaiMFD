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
		return { line, var.text.length(), var.ValueRoomOn(SimPage::DisplayLength) };
	}

	static std::array<StringSection, 3>	DissectLine(std::wstring& line, const DisplayVar& var)
	{
		StringSection text { line, 0, var.text.length() };
		StringSection num  = GetTargetField(line, var);
		StringSection unit = num.FollowedBy(var.unitText.length());

		return { text, num, unit };
	}


	static void PreformatReading(std::wstring& buffer, const DisplayVar& var)
	{
		auto [textTrg, _, unitTrg] = DissectLine(buffer, var);

		textTrg.FillWith(var.text);
		unitTrg.FillWith(var.unitText);
	}

#pragma endregion




#pragma region Page logic

	ReadoutScrollList::ReadoutScrollList(uint32_t id, const Dependencies& deps, std::vector<DisplayVar> dvars) :
		SimPage   { id, deps },
		variables { std::move(dvars) },
		scroller  { *this, std::max<unsigned>(3, variables.size()) }
	{
		uint32_t i = 0;
		for (const DisplayVar& var : variables)
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
		for (const DisplayVar& var : variables)
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
			const DisplayVar& var   = variables[i];
			StringSection	  field = GetTargetField(scroller.ModLine(i), var);

			PaddedAlignment aln { Align::Right, 1 };

			// TODO: switch all types - TypeMapping?
			switch (variables[i].definition.typeReqd)
			{
				case RequestType::Real:
					PlaceNumber(values[i].AsDouble(), var.decimalCount, field, aln);
					break;

				case RequestType::UnsignedInt:
					PlaceNumber(values[i].AsUnsigned32(), field, aln);
					break;

				case RequestType::SignedInt:
					DBG_BREAK_M ("NO OVERLOAD YET!");				// TODO
					PlaceNumber(values[i].AsInt64(), field, aln);
					break;

				case RequestType::String:
					DBG_BREAK_M ("NEEDS WSTRING CONVERSION!");		// TODO
					//PlaceText(values[i].AsString(), field, aln);
					break;

				default:
					DBG_BREAK;
			}
		}
	}


	void ReadoutScrollList::OnScroll(bool up, TimePoint)
	{
		if (up)	 scroller.ScrollDown();
		else	 scroller.ScrollUp();
	}

#pragma endregion


}	// namespace FSMfd::Pages
