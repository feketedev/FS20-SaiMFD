#include "ReceiveBuffer.h"

#include "Utils/Debug.h"



namespace FSMfd::SimClient 
{
	UniqueReceiveBuffer::UniqueReceiveBuffer(GroupId gid) :
		Group		{ gid },
		lastReceive { TimePoint::min() }
	{
	}


	void UniqueReceiveBuffer::Invalidate()
	{
		lastData.reset();
		lastReceive = TimePoint::min();
	}


	void UniqueReceiveBuffer::Receive(GroupId gid, const SimvarList& vars, TimePoint stamp)
	{
		DBG_ASSERT_M (Group == gid, "Duplicate subscription?");
		
		size_t msgLen = vars.DataDWords();
		if (buffer.size() < msgLen)
			buffer.resize(msgLen);

		lastData.emplace(vars.CopyValues(buffer.data()));
		lastReceive = stamp;
	}


	const SimvarList& UniqueReceiveBuffer::Get() const
	{
		DBG_ASSERT (HasData());

		return *lastData;
	}


}	// FSMfd::SimClient
