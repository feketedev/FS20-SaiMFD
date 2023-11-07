#pragma once

#include <atomic>
#include <limits>



namespace Utils
{

	/// Fully optimistic for reads, assuming writes are occasional.
	class LiteSharedLock
	{
		std::atomic_int		shareCount		= 0;		// < 0 during exclusive lock!
		std::atomic_bool	exclusiveAwaits = false;
		
		static constexpr int MinInt = std::numeric_limits<int>::min();


	public:
		// tolerable maximum, no checks
		static constexpr int MaxReaders = std::numeric_limits<int>::max();
		
		static_assert (MinInt <= -MaxReaders);


		void LockShared()		noexcept;
		void LockExclusive()	noexcept;
		bool TryLockShared()	noexcept;

		void UnlockShared()		noexcept;
		void UnlockExclusive()	noexcept;


		// STL compatibility (Shared- & BasicLockable)
		void lock_shared()		noexcept	{ LockShared();			  }
		void lock()				noexcept	{ LockExclusive();		  }
		void unlock_shared()	noexcept	{ UnlockShared();		  }
		void unlock()			noexcept	{ UnlockExclusive();	  }
		bool try_lock_shared()	noexcept	{ return TryLockShared(); }


		~LiteSharedLock();

	private:
		void WaitExclusiveFin() noexcept;
	};

}