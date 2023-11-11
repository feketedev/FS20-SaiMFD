#pragma once

#include "IStateDetector.h"
#include "DetectorHelpers.h"
#include "SimClient/DedupSimvarRegister.h"
#include <memory>



namespace FSMfd::Led {

	/* Available implementations of IStateDetector.	  *
	 * 												  *
	 * These are simplistic algorithms templated for  *
	 * prettier construction + performance.			  *
	 *												  *
	 * Use them in Configurator.cpp					  */


	// ==== Composites ==================================================================

	/* Note:										  *
	 * Composites should incorporate their nesteds by *
	 * templated value. This way the only allocation  *
	 * takes place at the root, when the complete	  *
	 * instance ultimatly gets stored in LedOverride, *
	 * finally erasing the complex, templated type.	  */


	/// Use detector #1 to enable the output of detector #2.
	/// @tparam G
	///		The gate: result > 0 enabled output of D.
	///				  Nothing or 0 forces overall output to stay Nothing.
	/// @tparam D
	/// 	Gated detector: provides overall result if enabled.
	template<class G, class D>
	struct Conditional final : public IStateDetector {
		G gate;
		D detector;

		static_assert(std::is_convertible_v<G*, IStateDetector*>
				   && std::is_convertible_v<D*, IStateDetector*>);
		

		void RegisterVariables(SimClient::DedupSimvarRegister& dp) override
		{
			gate.RegisterVariables(dp);
			detector.RegisterVariables(dp);
		}


		unsigned StateCount() const override
		{
			return detector.StateCount();
		}


		optional<unsigned>	DetectState(const SimClient::SimvarList& vars) const override
		{
			if (0 < gate.DetectState(vars))
				return detector.DetectState(vars);

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



	// ==== Utility =====================================================================

	/// Eliminates some boilerplate by CRTP.
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



	// ==== Numeric detectors ===========================================================

	/// Simple On/Off detector for an integer SimVar. (Anything non-0 yields state 1.)
	struct SwitchDetector final : public SinglevarDetectorBase<SwitchDetector> {
		SwitchDetector(const char* varName, const char* unit = "Bool") :
			SinglevarDetectorBase<SwitchDetector> { SimVarDef { varName, unit } }
		{
		}

		unsigned StateCount() const override
		{
			return 2; 
		}

		optional<unsigned> DetectState(const SimClient::SimvarList& simvars) const override
		{
			return simvars[VarIndex].AsUnsigned32() ? 1 : 0;
		}
	};



	/// Selects the containing interval for an integer/float SimVar's value.
	/// Specify N-1 boundaries for the N intervals.
	template<class T, unsigned N>
	class RangeDetector final : public SinglevarDetectorBase<RangeDetector<T, N>> {
		const Boundaries<T, N - 1>	bounds;

		static_assert (std::is_arithmetic_v<T> && !std::is_pointer_v<T>, "Strings are not supported here.");
		static_assert (0 < N, "Constant 0 state makes no sense.");

	public:
		RangeDetector(const char* varName, const char* unit, Boundaries<T, N - 1> bounds) :
			SinglevarDetectorBase<RangeDetector<T, N>> {
				SimVarDef { varName, unit, RequestTypeFor<T>::value }
			},
			bounds { std::move(bounds) }
		{
		}


		unsigned StateCount() const override
		{
			return N;
		}


		optional<unsigned> DetectState(const SimClient::SimvarList& simvars) const override
		{
			constexpr auto accessor = RequestTypeFor<T>::accessor;
			auto           value = (simvars[this->VarIndex].*accessor)();

			unsigned s = 0;
			while (s < bounds.size() && bounds[s].ExceededBy(value))
				s++;

			return s;
		}
	};

	template <class T, unsigned N>
	RangeDetector(const char*, const char*, Boundaries<T, N>) -> RangeDetector<T, N + 1>;



	// ==== Universal detectors =========================================================

	/// States triggered by discrete values (of any supported type).
	/// Results Nothing when no trigger values hit.
	template<class T, unsigned N>
	class DiscreteDetector final : public SinglevarDetectorBase<DiscreteDetector<T, N>> {
		const TriggerValues<T, N>	triggers;

		static_assert (0 < N, "Constant undefined state makes no sense.");

	public:
		DiscreteDetector(const char* varName, const char* unit, TriggerValues<T, N> triggers) :
			SinglevarDetectorBase<DiscreteDetector<T, N>> { 
				SimVarDef { varName, unit, RequestTypeFor<T>::value }
			},
			triggers { std::move(triggers) }
		{
		}

		unsigned StateCount() const override 
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


		optional<unsigned> DetectState(const SimClient::SimvarList& simvars) const override
		{
			constexpr auto accessor = RequestTypeFor<T>::accessor;
			auto		   value = (simvars[this->VarIndex].*accessor)();

			unsigned s = 0;
			while (s < triggers.size() && !AreEqual<T>(triggers[s], value))
				s++;

			if (s < triggers.size())
				return s;

			return Nothing;
		}
	};

	template <class T, unsigned N>
	DiscreteDetector(const char*, const char*, TriggerValues<T, N>) -> DiscreteDetector<T, N>;


}	// namespace FSMfd::Led
