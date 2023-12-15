#include "ColumnGauge.h"

#include "DirectOutputHelper/X52Page.h"		// DisplayLen
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace Utils::String;


	static std::vector<SimvarPrinter> CreatePrinters(const std::vector<SimVarDef>& vars, DecimalUsage decimals)
	{
		std::vector<SimvarPrinter> res;
		for (const SimVarDef& sv : vars)
			res.push_back(CreatePrinterFor(sv.typeReqd, decimals));
		return res;
	}


	ColumnGauge::ColumnGauge(std::wstring header, std::vector<SimVarDef> vars, DecimalUsage decimals) :
		StackableGauge { static_cast<unsigned>(header.length()),
						 static_cast<unsigned>(vars.size() + 1),
						 std::move(vars)						},
		header		   { std::move(header) },
		printers	   { CreatePrinters(this->Variables, decimals) }
	{
		LOGIC_ASSERT (!this->header.empty());
		LOGIC_ASSERT (!this->Variables.empty());
	}


	ColumnGauge::ColumnGauge(std::wstring header, std::vector<SimVarDef> vars, SignUsage sign) :
		ColumnGauge { std::move(header), std::move(vars), DecimalUsage { 2, sign } }
	{
	}


	ColumnGauge::ColumnGauge(unsigned width, std::vector<SimVarDef> vars, DecimalUsage decimals) :
		StackableGauge { width, static_cast<unsigned>(vars.size()), std::move(vars) },
		printers	   { CreatePrinters(this->Variables, decimals) }
	{
		LOGIC_ASSERT (!this->Variables.empty());
	}

	
	ColumnGauge::ColumnGauge(unsigned width, std::vector<SimVarDef> vars, SignUsage sign) :
		ColumnGauge { width, std::move(vars), DecimalUsage { 2, sign } }
	{
	}


	
	
	void ColumnGauge::Clean(DisplayArea& display)
	{
		DBG_ASSERT (display.size == Variables.size() + HasHeader());

		unsigned row = 0;
		if (HasHeader())
			display[row++].FillIn(header);

		while (row < display.size)
			display[row++].FillWith(L' ');
	}


	void ColumnGauge::Update(const SimvarSublist& measurement, DisplayArea& display)
	{
		DBG_ASSERT (display.size == Variables.size() + HasHeader());
		DBG_ASSERT (measurement.VarCount == Variables.size());

		for (size_t i = 0; i < Variables.size(); i++)
		{
			const SimvarPrinter& print = printers[i];
			print(measurement[i], display[i + HasHeader()], Align::Right);
		}
	}


}	// namespace FSMfd::Pages
