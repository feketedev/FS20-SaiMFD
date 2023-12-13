#pragma once

#include "FSMfdTypes.h"
#include <vector>



namespace FSMfd::SimClient
{
	
	// ----- Callback interfaces ----------------------------------------------------

	class IDataReceiver {
	public:
		virtual void Receive(GroupId, const SimvarList&, TimePoint stamp) = 0;

		virtual ~IDataReceiver();
	};


	class IEventReceiver {
	public:
		virtual void Notify(NotificationCode, uint32_t parameter, TimePoint stamp) = 0;
		virtual void Notify(NotificationCode, const char* string, TimePoint stamp) = 0;

		virtual ~IEventReceiver();
	};



	// ----- DataReceiver Helpers ---------------------------------------------------

	/// A variable group as received from SimConnect. 
	/// Valid under Receive call. Use @a CopyValues to store.
	class SimvarList {
		const size_t			varCount;
		const size_t*	const	positions;		// last denotes end of data
		const uint32_t*	const	data;

	public:
		VarIdx VarCount()	const	{ return varCount; }
		size_t DataDWords()	const	{ return positions[varCount]; }

		SimvarValue operator[](VarIdx) const;

		/// Quickly save received variables for potential later use.
		/// @param buffer:	at least @a DataDWords long target
		/// @returns		a copy backed by @p buffer, 
		///					valid until group definition change - dangling afterwards!
		SimvarList [[nodiscard]] CopyValues(uint32_t* buffer) const;

		SimvarList(const std::vector<size_t>&, const uint32_t*);
		SimvarList(size_t, const size_t*, const uint32_t*);
	};



	/// Ready-to-read Simulation Variable - provided you know its type.
	struct SimvarValue {
		const uint32_t* const	myData;
		const uint32_t* const	myEnd;

		uint32_t	AsUnsigned32()	const;
		uint64_t	AsUnsigned64()	const;
		int32_t		AsInt32()		const;
		int64_t		AsInt64()		const;
		float		AsSingle()		const;
		double		AsDouble()		const;

		pair<const char*, size_t>	AsStringBuffer() const;
		std::string_view			AsString()		 const;
	};


}	// namespace FSMfd::SimClient
