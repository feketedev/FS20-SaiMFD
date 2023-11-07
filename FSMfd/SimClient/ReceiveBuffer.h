#pragma once

#include "IReceiver.h"



namespace FSMfd::SimClient
{

	class UniqueReceiveBuffer final : public IDataReceiver
	{
		TimePoint				lastReceive;
		optional<SimvarList>	lastData;
		std::vector<uint32_t>	buffer;

	public:
		const GroupId			Group;

		UniqueReceiveBuffer(GroupId);
		UniqueReceiveBuffer(const UniqueReceiveBuffer&) = delete;

		bool	  HasData()		 const	{ return lastData.has_value(); }
		TimePoint LastReceived() const	{ return lastReceive; }
		
		const SimvarList& Get()	 const;
		
		void Invalidate();

		// IDataReceiver
		void Receive(GroupId, const SimvarList&, TimePoint) override;
	};


}	// namespace FSMfd::SimClient
