#include "SimPage.h"

#include "SimClient/FSClient.h"
#include "Utils/Debug.h"



namespace FSMfd::Pages
{
	using namespace SimClient;


	SimPage::SimPage(uint32_t id, const Dependencies& deps) :
		Page            { id },
		ContentAgeLimit { deps.ContentAgeLimit },
		SimClient       { deps.SimClient },
		simValues       { deps.SimClient.CreateVarGroup() }
	{
	}


	// NOTE: also guards RegisterSimVars in descendant ctor
	SimPage::~SimPage()
	{
		SimClient.TryClearVarGroup(simValues.Group);
	}


	void SimPage::RegisterSimVar(const SimVarDef& var)
	{
		VarIdx vid = SimClient.AddVar(simValues.Group, var);
		LOGIC_ASSERT_M (vid == simvarCount++, "Duplicate SimVar group ID?");
	}


	void SimPage::SetUpdateFrequency(SimClient::UpdateFrequency freq)
	{
		updateFreq = freq;
		if (vargroupEnabled)
		{
			// MAYBE: SimConnect could set frequency without disabling first
			SimClient.DisableVarGroup(simValues.Group);
			SimClient.EnableVarGroup(simValues.Group, *this, freq);
		}
	}


	void SimPage::OnActivate(TimePoint t)
	{
		const bool outdated = HasOutdatedData(t);
		
		if (outdated || !vargroupEnabled)
			CleanContent();

		if (outdated)
			simValues.Invalidate();

		if (!vargroupEnabled)
		{
			DBG_ASSERT (simvarCount);
			SimClient.EnableVarGroup(simValues.Group, *this, updateFreq);
			vargroupEnabled = true;
		}
		// NOTE: no DrawLines yet, immediate Update can follow!
	}


	void SimPage::OnDeactivate(TimePoint)
	{
		if (!BackgroundReceiveEnabled && vargroupEnabled)
		{
			SimClient.DisableVarGroup(simValues.Group);
			vargroupEnabled = false;
		}
	}


	void SimPage::Update(TimePoint at)
	{
		if (HasOutdatedData(at))
		{
			CleanContent();
			if (simValues.HasData())
			{
				simValues.Invalidate();
				Debug::Warning("Sim data not received in time!");
			}
		}
		else if (HasPendingUpdate())
		{
			UpdateContent(simValues.Get());
			lastUpdate = simValues.LastReceived();
		}
	}


	bool SimPage::HasOutdatedData(TimePoint at) const
	{
		return updateFreq != UpdateFrequency::OnValueChange 
			&& simValues.HasData()
			&& simValues.LastReceived() + ContentAgeLimit < at;
	}


	void SimPage::Animate(unsigned steps)
	{
		StepAnimation(steps);
	}


	void SimPage::Receive(GroupId gid, const SimvarList& data, TimePoint stamp)
	{
		DBG_ASSERT_M (gid == simValues.Group, "Currently a single group per Page is expected.");

		simValues.Receive(gid, data, stamp);
		if (BackgroundReceiveEnabled || IsAwaitingResponse())
		{
			UpdateContent(data);
			lastUpdate = stamp;
		}
	}


}	// namespace FSMfd::Pages
