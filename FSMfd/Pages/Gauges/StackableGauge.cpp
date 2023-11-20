#include "StackableGauge.h"

#include "SimVarDef.h"
#include "SimClient/IReceiver.h"    // SimvarList
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
    using namespace SimClient;


    StackableGauge::~StackableGauge() = default;


    StackableGauge::StackableGauge(unsigned width, unsigned height, std::vector<SimVarDef>&& vars) :
        DisplayWidth  { width },
        DisplayHeight { height },
        Variables     { std::move(vars) }
    {
    }


    SimvarSublist::SimvarSublist(const SimvarList& origin, VarIdx start, VarIdx len) :
        list     { origin },
        start    { start },
        VarCount { len }
    {
        DBG_ASSERT (start + len <= origin.VarCount());
    }


    SimvarValue SimvarSublist::operator[](VarIdx iVirt) const
    {
        DBG_ASSERT (iVirt < VarCount);

        return list[start + iVirt];
    }


}	// namespace FSMfd::Pages
