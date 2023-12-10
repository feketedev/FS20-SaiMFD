#include "SwitchGauge.h"
#include "SimVarDef.h"
#include "SimClient/IReceiver.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	static unsigned SumWidth(const std::wstring title, const SwitchGauge::SideSymbols& symbs)
	{
		return static_cast<unsigned>(title.size())
			 + symbs.first.has_value()
			 + symbs.second.has_value();
	}


	static SimVarDef PassToSimvar(std::string& name)
	{
		return { std::move(name), "Bool" };
	}


	SwitchGauge::SwitchGauge(std::wstring title, std::string simvarName, SideSymbols symbs) :
		StackableGauge { SumWidth(title, symbs), 1, { PassToSimvar(simvarName) } },
		title		   { std::move(title) },
		leftSymbol     { symbs.first },
		rightSymbol    { symbs.second }
	{
	}


	void SwitchGauge::Clean(DisplayArea& display)
	{
		display[0].FillWith(L' ');
		GetTitleField(display).FillIn(title);
	}


	void SwitchGauge::Update(const SimvarSublist& state, DisplayArea& display)
	{
		DBG_ASSERT (state.VarCount == 1);

		bool on = state[0].AsUnsigned32() > 0;

		if (leftSymbol.has_value())
			*display[0].GetStart() = on ? *leftSymbol : L' ';
		if (rightSymbol.has_value())
			*display[0].GetLast() = on ? *rightSymbol : L' ';
	}


	Utils::String::StringSection SwitchGauge::GetTitleField(DisplayArea& display) const
	{
		DBG_ASSERT (display.size == 1);

		unsigned offset = leftSymbol.has_value();
		unsigned negOff = rightSymbol.has_value();

		return display[0].SubSection(offset, DisplayWidth - offset - negOff);
	}


}	// namespace FSMfd::Pages
