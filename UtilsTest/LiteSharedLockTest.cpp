
	/*  Part of FS20-SaiMFD			   *
	 *  Copyright 2023 Norbert Fekete  *
     *                                 *
	 *  Released under GPLv3.		   */


#include "CppUnitTest.h"

#include "Utils/LiteSharedLock.h"
#include <thread>
#include <future>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <array>



using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::chrono_literals;


namespace FSMfdTests
{

#pragma region Helpers

	template<class ThreadList, class R, class P>
	static bool WaitThreadsTimeout(ThreadList& threads, const std::chrono::duration<R, P>& maxWait)
	{
		std::mutex				finMutex;
		std::condition_variable finCond;

		// some hacking around having no thread::terminate()
		std::unique_ptr<bool> timeoutFlagHolder = std::make_unique<bool>(false);
		bool& timeoutFlag = *timeoutFlagHolder;

		std::thread joiner { [&, timeoutFlag = move(timeoutFlagHolder)]
		{
			for (std::thread& t : threads)
			{
				t.join();
				if (*timeoutFlag)
					return;
			}
			finCond.notify_one();
		} };

		std::unique_lock ulk { finMutex };
		auto st = finCond.wait_for(ulk, maxWait);

		if (st == std::cv_status::no_timeout)
		{
			joiner.join();
			return true;
		}

		// This branch is dirty testcode only, not to have crash after break points!
		// It is not really correct :)
		timeoutFlag = true;
		joiner.detach();
		return false;
	}


	template <class TaskFun, class C, class D>
	static std::vector<std::thread>
	SpawnThreadsSyncStart(size_t count, std::chrono::time_point<C, D> tstart, TaskFun&& task)
	{
		auto runner = [=](int tid)
		{
			std::this_thread::sleep_until(tstart);
			task(tid);
		};

		std::vector<std::thread> threads;
		threads.reserve(count);

		for (int t = 0; t < count; t++)
			threads.emplace_back(runner, t);

		auto creationMargin = tstart - std::chrono::high_resolution_clock::now();
		LogDuration("Thread creation margin:\t", creationMargin);

		return threads;
	}


	template<class R, class P>
	static void LogDuration(const char* title, std::chrono::duration<R, P>& t)
	{
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (t).count();
		std::stringstream msg;
		msg << title << ' ' << ms << " ms.\n";
		Logger::WriteMessage(msg.str().c_str());
	}

#pragma endregion



	TEST_CLASS(LiteSharedLockTest)
	{
	public:
		using LiteSharedLock = Utils::LiteSharedLock;
		using Clock			 = std::chrono::high_resolution_clock;
		
		TEST_METHOD(UncontendedPass)
		{
			LiteSharedLock lk;

			lk.LockShared();
			Assert::IsTrue(lk.TryLockShared());
			lk.UnlockShared();
			lk.UnlockShared();

			lk.LockExclusive();
			Assert::IsFalse(lk.TryLockShared());
			lk.UnlockExclusive();
			
			Assert::IsTrue(lk.TryLockShared());
			lk.UnlockShared();
		}


		TEST_METHOD(SharedDeniesExclusive)
		{
			LiteSharedLock lk;

			lk.LockShared();

			volatile bool flag = false;
			std::thread thread { [&]()
			{
				lk.LockExclusive();
				flag = true;
				lk.UnlockExclusive();
			} };
			std::this_thread::sleep_for(10ms);
			Assert::IsFalse(flag);

			lk.UnlockShared();
			thread.join();
			Assert::IsTrue(flag);
		}


		TEST_METHOD(ExclusiveDeniesShared)
		{
			LiteSharedLock lk;

			lk.LockExclusive();

			volatile bool flag = false;
			std::thread thread { [&]()
			{
				lk.LockShared();
				flag = true;
				lk.UnlockShared();
			} };
			std::this_thread::sleep_for(10ms);
			Assert::IsFalse(flag);

			lk.UnlockExclusive();
			thread.join();
			Assert::IsTrue(flag);
		}


		TEST_METHOD(Exclusiveness)
		{
			LiteSharedLock lk;

			lk.LockExclusive();

			volatile bool flag = false;
			std::thread thread { [&]()
			{
				lk.LockExclusive();
				flag = true;
				lk.UnlockExclusive();
			} };
			std::this_thread::sleep_for(10ms);
			Assert::IsFalse(flag);

			lk.UnlockExclusive();
			thread.join();
			Assert::IsTrue(flag);
		}

		TEST_METHOD(HeavyContention)
		{
			LiteSharedLock llk;
			
			TestContentionIntArray(llk);

			Logger::WriteMessage("\nReference times with std::shared_mutex:\n\n");

			std::shared_mutex referenceLk;
			TestContentionIntArray(referenceLk);
		}


		template <class Lock>
		static void TestContentionIntArray(Lock& lk)
		{
			constexpr int Threads		 = 10;
			constexpr int ItemsPerThread = 1000;
			constexpr int Size = Threads * ItemsPerThread;

			volatile int data[Size] = { 0 };

			// 1. Test exclusive access via read+write filling the array.
			{
				auto writeNext = [&](size_t /*tid*/)
				{
					int i = 0;
					for (int p = 0; p < ItemsPerThread; p++)
					{
						std::unique_lock xguard { lk };
						while (data[i] > 0)
							i++;

						Assert::IsTrue(i < Size);	// as we launch no more thread
						data[i] = i + 1;
					}
				};

				// sync their start for maximal load on the lock
				auto tstart = Clock::now() + 50ms;
				auto writerThreads = SpawnThreadsSyncStart(Threads, tstart, writeNext);
				
				Assert::IsTrue(WaitThreadsTimeout(writerThreads, 5s));

				LogDuration("Contended writes took: \t\t\t", Clock::now() - tstart);

				for (int i = 0; i < Size; i++)
					Assert::AreEqual(i + 1, (int)data[i]);
			}

			// 2. Test simultaneous reads with no disturbance
			{
				int readData[Size];		// should become a copy

				auto readChunk = [&](size_t tid)
				{
					size_t start = tid * ItemsPerThread;

					for (int i = 0; i < ItemsPerThread; i++)
					{
						std::shared_lock guard { lk };

						size_t pos = start + i;
						readData[pos] = data[pos];
					}
				};

				// sync their start for maximal load on the lock
				auto tstart = Clock::now() + 50ms;
				auto readerThreads = SpawnThreadsSyncStart(Threads, tstart, readChunk);

				Assert::IsTrue(WaitThreadsTimeout(readerThreads, 1s));

				LogDuration("Simultaneous reads took:\t\t", Clock::now() - tstart);

				for (int i = 0; i < Size; i++)
					Assert::AreEqual((int)data[i], readData[i]);
			}

			// 3. Simultaneous reads disturbed by some writes
			{
				//// Reads will copy again, but proceeding head-to-head.
				//// Writes will set -1, at some places.
				volatile int readData[Size];

				auto read = [&](size_t tid)
				{
					const size_t off = tid;

					for (int i = 0; i < ItemsPerThread; i++)
					{
						size_t pos = off + i * Threads;

						std::shared_lock guard { lk };
						readData[pos] = data[pos];
					}
				};

				constexpr int WThreads = 2;
				constexpr int WOffs	   = Threads / WThreads;
				auto disturb = [&](size_t tid)
				{
					const size_t off = tid * WOffs;

					for (int i = 0; i < ItemsPerThread; i++)
					{
						size_t pos = off + i * Threads;

						std::unique_lock xguard { lk };
						data[pos] = -1;
					}
				};

				auto tstart = Clock::now() + 50ms;
				auto readThreads	= SpawnThreadsSyncStart(Threads,  tstart, read);
				auto disturbThreads	= SpawnThreadsSyncStart(WThreads, tstart, disturb);

				Assert::IsTrue(WaitThreadsTimeout(readThreads,	  2s));
				Assert::IsTrue(WaitThreadsTimeout(disturbThreads, 2s));

				LogDuration("Contended reads+writes took:\t", Clock::now() - tstart);

				for (int i = 0; i < Size; i++)
				{
					const int origData = i + 1;

					const int block = i / ItemsPerThread;
					const int elem  = i % Threads;

					if (elem % WOffs == 0)		// disturbed elem
					{
						Assert::AreEqual(-1, (int)data[i]);
						Assert::IsTrue(readData[i] == -1 || readData[i] == origData);
					}
					else
					{
						Assert::AreEqual(origData, (int)data[i]);
						Assert::AreEqual(origData, (int)readData[i]);
					}
				}

				int writes = 0;
				int disturbances = 0;
				for (int i = 0; i < Size; i++)
				{
					if (data[i] == -1)
					{
						writes++;
						disturbances += (readData[i] != -1);
					}
				}
				Assert::AreEqual(WThreads * ItemsPerThread, writes);
				Assert::IsTrue(disturbances <= writes);

				std::stringstream msg;
				msg << "Writes preceeded reads:\t " << disturbances << '/' << writes << " times.\n";
				Logger::WriteMessage(msg.str().c_str());
			}
		}
	};
}
