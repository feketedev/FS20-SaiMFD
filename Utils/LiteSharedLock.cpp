#include "LiteSharedLock.h"
#include "Debug.h"
#include <thread>



namespace Utils
{
	static constexpr int MaxSpinsShared				 = 100;
	static constexpr int MaxSpinsExclusives			 = 100;
	static constexpr int MaxSpinsExclusiveForShareds = 2000;

//	void Test() { ; }

	void LiteSharedLock::LockShared() noexcept
	{
		while (true)
		{
			// enter optimistically - above 0: no exclusive "writer"
			int ticket = ++shareCount;
			if (ticket > 0)
				return;

			// below 0 a writer awaits, shared "readers" should cool down
			DBG_ASSERT (exclusiveAwaits);
			--shareCount;
			std::this_thread::yield();

			// wait for the write to finish, then retry
			WaitExclusiveFin();
		}
	}


	void LiteSharedLock::UnlockShared() noexcept
	{
		--shareCount;
	}


	void LiteSharedLock::LockExclusive() noexcept
	{
		// 1. acquire exclusion privilege
		{
			bool mine;
			bool expect = false;

			while (!(mine = exclusiveAwaits.compare_exchange_strong(expect, true)))
			{
				expect = false;
				WaitExclusiveFin();
			}
		}

		// 2. signal readers
		int ticket = shareCount += MinInt;
		DBG_ASSERT (ticket < 0);

		// 3. active readers are free to finish, wait them
		do
		{
			// New read trials can anytime disturb the ticket,
			// but once MinInt was reached, we know that all
			// readers are just attempting to lock.
			for (int s = 0; ticket > MinInt && s < MaxSpinsExclusiveForShareds; ++s)
				ticket = shareCount;

			std::this_thread::yield();
		}
		while (ticket > MinInt);
	}


	bool LiteSharedLock::TryLockShared() noexcept
	{
		int ticket = ++shareCount;
		if (ticket > 0)
			return true;

		--shareCount;
		return false;
	}


	void LiteSharedLock::UnlockExclusive() noexcept
	{
		DBG_ASSERT (shareCount < 0 && exclusiveAwaits);
		shareCount += MinInt;
		exclusiveAwaits = false;
	}


	void LiteSharedLock::WaitExclusiveFin() noexcept
	{
		bool occup = exclusiveAwaits;
		while (occup)
		{
			for (int s = 0; occup && s < MaxSpinsShared; ++s)
				occup = exclusiveAwaits;

			if (occup)
				std::this_thread::yield();
		}
	}


	LiteSharedLock::~LiteSharedLock()
	{
		DBG_ASSERT (shareCount == 0);
	}

}	// namespace Utils