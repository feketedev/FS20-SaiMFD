#include "IReceiver.h"

#include "Utils/Debug.h"



namespace FSMfd::SimClient
{

	IDataReceiver::~IDataReceiver() = default;
	IEventReceiver::~IEventReceiver() = default;




#pragma region SimvarList

	SimvarList::SimvarList(const std::vector<size_t>& poss, const uint32_t* data) :
		SimvarList { poss.size() - 1, poss.data(), data }
	{
		DBG_ASSERT (!poss.empty());
	}


	SimvarList::SimvarList(size_t count, const size_t* poss, const uint32_t* data) :
		varCount  { count },
		positions { poss },
		data      { data }
	{
	}


	SimvarList SimvarList::CopyValues(uint32_t* buffer) const
	{
		memcpy(buffer, data, DataDWords() * sizeof(uint32_t));

		return { varCount, positions, buffer };
	}


	SimvarValue SimvarList::operator[](VarIdx i) const
	{
		DBG_ASSERT (i < VarCount());

		size_t pos  = positions[i];
		size_t next = positions[i + 1];

		return { data + pos, data + next };
	}

#pragma endregion



#pragma region SimvarValue

	template <class T>
	const T& Checked(const uint32_t* dat, const uint32_t* end)
	{
		DBG_ASSERT_M (   reinterpret_cast<const char*> (dat) + sizeof(T) 
					  <= reinterpret_cast<const char*> (end), "Wrong type requested?");

		return *reinterpret_cast<const T*>(dat);
	}



	uint32_t SimvarValue::AsUnsigned32() const
	{
		return Checked<uint32_t>(myData, myEnd);
	}


	uint64_t SimvarValue::AsUnsigned64() const
	{
		return Checked<uint64_t>(myData, myEnd);
	}



	int32_t SimvarValue::AsInt32() const
	{
		return Checked<int32_t>(myData, myEnd);
	}


	int64_t SimvarValue::AsInt64() const
	{
		return Checked<int64_t>(myData, myEnd);
	}



	float SimvarValue::AsSingle() const
	{
		return Checked<float>(myData, myEnd);
	}


	double SimvarValue::AsDouble() const
	{
		return Checked<double>(myData, myEnd);
	}



	pair<const char*, size_t>	SimvarValue::AsStringBuffer() const
	{
		size_t dataCells = myEnd - myData;
		return {
			reinterpret_cast<const char*> (myData),
			dataCells * sizeof(*myData)
		};
	}


	std::string_view SimvarValue::AsString() const
	{
		const auto [buff, size] = AsStringBuffer();
		size_t len = strnlen(buff, size);
		return { buff, len };
	}

#pragma endregion


}	// namespace FSMfd::SimClient
