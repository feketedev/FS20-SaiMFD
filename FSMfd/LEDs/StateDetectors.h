#pragma once

#include "IStateDetector.h"
#include "DetectorHelpers.h"
#include "SimClient/DedupSimvarRegister.h"
#include <memory>



namespace FSMfd::Led {

	// templated for prettier construction + to inline most calls
	template<class G, class D>
	struct Conditional final : public IStateDetector {
		G gate;			// enable: state > 0
		D detector;		// gated source of result state

		static_assert(std::is_convertible_v<G*, IStateDetector*>
				   && std::is_convertible_v<D*, IStateDetector*>);

		

		void		RegisterVariables(SimClient::DedupSimvarRegister& dp) override
		{
			gate.RegisterVariables(dp);
			detector.RegisterVariables(dp);
		}


		unsigned	StateCount() const override
		{
			return detector.StateCount();
		}


		optional<unsigned>	DeterminState(const SimClient::SimvarList& vars) const override
		{
			if (0 < gate.DeterminState(vars))
				return detector.DeterminState(vars);
				
			return Nothing;
		}

		std::unique_ptr<IStateDetector> Clone() const override
		{
			return std::make_unique<Conditional>(*this);
		}


		Conditional(G gate, D detector)
			: gate { std::move(gate) }, detector { std::move(detector) }
		{
		}
	};



	// ----------------------------------------------------------------------------------

	template<class Descendant>
	class SinglevarDetectorBase : public IStateDetector {
	protected:
		const SimVarDef		VarDefinition;
		SimClient::VarIdx	VarIndex;

		SinglevarDetectorBase(const SimVarDef& def) : VarDefinition { def }
		{
			VarIndex = std::numeric_limits<SimClient::VarIdx>::max();
		}

	public:
		void RegisterVariables(SimClient::DedupSimvarRegister& dedup) override final
		{
			VarIndex = dedup.Add(VarDefinition);
		}

		std::unique_ptr<IStateDetector> Clone() const override final
		{
			return std::make_unique<Descendant>(static_cast<const Descendant&>(*this));
		}
	};



	// ----------------------------------------------------------------------------------

	struct SwitchDetector final : public SinglevarDetectorBase<SwitchDetector> {
		SwitchDetector(const char* varName, const char* unit = "Bool") :
			SinglevarDetectorBase<SwitchDetector> { SimVarDef { varName, unit } }
		{
		}

		unsigned			StateCount() const override
		{
			return 2; 
		}

		optional<unsigned>	DeterminState(const SimClient::SimvarList& simvars) const override
		{
			return simvars[VarIndex].AsUnsigned32() ? 1 : 0;
		}
	};



	// ----------------------------------------------------------------------------------

	template<class T, unsigned N>
	class DiscreteDetector final : public SinglevarDetectorBase<DiscreteDetector<T, N>> {
		const Triggers<T, N>	triggers;

		static_assert (0 < N, "A constant undefined state makes no sense.");

	public:
		DiscreteDetector(const char* varName, const char* unit, Triggers<T, N> triggers) :
			SinglevarDetectorBase<DiscreteDetector<T, N>> { 
				SimVarDef { varName, unit, RequestTypeFor<T>::value }
			},
			triggers { std::move(triggers) }
		{
		}

		unsigned			StateCount() const override 
		{
			return N; 
		}


		template <class T1>
		static bool AreEqual(const T1& a, const T1& b, std::enable_if_t<RequestTypeFor<T1>::value != RequestType::String>* = nullptr)
		{
			return a == b;
		}


		template <class T1>
		static bool AreEqual(const T1& a, const T1& b, std::enable_if_t<RequestTypeFor<T1>::value == RequestType::String>* = nullptr)
		{
			return strcmp(a, b) == 0;
		}


		optional<unsigned>	DeterminState(const SimClient::SimvarList& simvars) const override
		{
			constexpr auto accessor = RequestTypeFor<T>::accessor;
			auto		   value = (simvars[this->VarIndex].*accessor)();

			size_t s = 0;
			while (s < triggers.size() && !AreEqual<T>(triggers[s], value))
				s++;

			if (s < triggers.size())
				return s;

			return Nothing;
		}
	};

	template <class T, unsigned N>
	DiscreteDetector(const char*, const char*, Triggers<T, N>) -> DiscreteDetector<T, N>;



	// ----------------------------------------------------------------------------------

	template<class T, unsigned N>
	class RangeDetector final : public SinglevarDetectorBase<RangeDetector<T, N>> {
		const Limits<T, N - 1>	limits;

		static_assert (std::is_arithmetic_v<T> && !std::is_pointer_v<T>, "Strings are not supported here.");
		static_assert (1 < N, "A single constant state makes no sense.");

	public:
		RangeDetector(const char* varName, const char* unit, Limits<T, N - 1> limits) :
			SinglevarDetectorBase<RangeDetector<T, N>> {
				SimVarDef { varName, unit, RequestTypeFor<T>::value }
			},
			limits { std::move(limits) }
		{
		}


		unsigned			StateCount() const override
		{
			return N;
		}


		optional<unsigned>	DeterminState(const SimClient::SimvarList& simvars)	const override
		{
			constexpr auto accessor = RequestTypeFor<T> ::accessor;
			auto           value = (simvars[this->VarIndex].*accessor)();

			size_t s = 0;
			while (s < limits.size() && limits[s].ExceededBy(value))
				s++;

			return s;
		}
	};

	template <class T, unsigned N>
	RangeDetector(const char*, const char*, Limits<T, N>) -> RangeDetector<T, N + 1>;


}	// namespace FSMfd::Led
