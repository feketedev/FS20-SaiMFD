#include "DedupSimvarRegister.h"

#include "SimClient/FSClient.h"
#include "Utils/Debug.h"



namespace FSMfd::SimClient
{

	DedupSimvarRegister::DedupSimvarRegister(FSClient& client) :
		client { client },
		Group  { client.CreateVarGroup() }
	{
	}


	DedupSimvarRegister::DedupSimvarRegister(FSClient& client, GroupId gid) :
		client { client },
		Group  { gid }
	{
	}


	VarIdx DedupSimvarRegister::Add(const SimVarDef& def)
	{
		VarIdx existing = 0;
		while (existing < definitions.size() && definitions[existing] != def)
			existing++;

		if (existing < definitions.size())
			return existing;

		const VarIdx idx = client.AddVar(Group, def);
		LOGIC_ASSERT (idx == definitions.size());
		definitions.push_back(def);
		return idx;
	}


}	// namespace FSMfd::SimClient
