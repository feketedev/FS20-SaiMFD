#pragma once

#include "FSMfdTypes.h"
#include "SimClient/FSClientTypes.h"
#include <vector>



namespace FSMfd::SimClient {

	class DedupSimvarRegister {
		FSClient&				client;
		std::vector<SimVarDef>	definitions;
	
	public:
		GroupId 				Group;
		
		DedupSimvarRegister(FSClient&);
		DedupSimvarRegister(FSClient&, GroupId);
		
		VarIdx	Add(const SimVarDef&);
	};


}	// namespace FSMfd::SimClient
