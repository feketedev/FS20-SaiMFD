#include "DirectOutputHelper/DirectOutputInstance.h"
#include "DirectOutputHelper/X52Page.h"
#include "CppUnitTest.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace DOHelper;


namespace DirectOutputHelperTest
{

	TEST_CLASS (X52PagingTest)
	{
		DirectOutputInstance directOutput;
		X52Output			 x52;

		X52Output::Page		 testPage1;
		X52Output::Page		 testPage2;
		X52Output::Page		 testPage3;

		unsigned			 drawStep = 1;

	public:
		X52PagingTest() : 
			directOutput { L"Test plugin" }, 
			x52          { SelectDevice() },
			testPage1    { 1 },
			testPage2    { 2 },
			testPage3    { 3 }
		{
			testPage1.SetLine(0, L" -- TEST 1 -- ");
			testPage2.SetLine(0, L" -- TEST 2 -- ");
			testPage3.SetLine(0, L" -- TEST 3 -- ");
		}


		X52Output SelectDevice()
		{
			for (const SaiDevice& dev : directOutput.EnumerateFreeDevices())
			{
				if (dev.Type == SaiDeviceType::X52Pro)
					return directOutput.UseX52(dev);
			}
			Assert::Fail(L"No free X52 device currently attached.");
		}

		
		void AssertDraw(X52Output::Page& pg)
		{
			Assert::IsTrue(&pg == x52.GetActivePage());
			pg.SetLine(2, L"    Step " + std::to_wstring(drawStep++));
			Assert::IsTrue(pg.IsDirty());
			pg.DrawLines();
			Assert::IsFalse(pg.IsDirty());
		}



		TEST_METHOD_CLEANUP (Reset) 
		{
			x52.ClearPages();
			drawStep = 1;
			testPage1.SetLine(1, L"");
			testPage1.SetLine(2, L"");
			testPage2.SetLine(1, L"");
			testPage2.SetLine(2, L"");
			testPage3.SetLine(1, L"");
			testPage3.SetLine(2, L"");
		}


		TEST_METHOD (BackgroundAddRemove)
		{
			AddBackground(testPage1);
			Assert::IsFalse(WaitEventFor(500ms));
			testPage1.Remove();
			Assert::IsFalse(WaitEventFor(500ms));

			AddForeground(testPage2);
			Assert::IsFalse(WaitEventFor(500ms));
			AssertDraw(testPage2);

			AddBackground(testPage1);
			Assert::IsFalse(WaitEventFor(500ms));
			testPage1.Remove();
			Assert::IsFalse(testPage1.IsActive() || testPage1.IsAdded());
			std::this_thread::sleep_for(1s);
			Assert::IsFalse(WaitEventFor(500ms));

			AssertDraw(testPage2);
			
			Assert::IsFalse(WaitEventFor(1s));
			AssertDraw(testPage2);
			Assert::IsFalse(WaitEventFor(1s));
		}


		TEST_METHOD (AddRemoveStacked)
		{
			AddAll();

			for (auto pg : { &testPage3, &testPage2, &testPage1 })
			{
				pg->Remove();
				Assert::IsTrue(nullptr == x52.GetActivePage());
				Assert::IsFalse(pg->IsActive() || pg->IsAdded());
				Assert::IsFalse(WaitEventFor(500ms));
			}


			// -- Now using next remaining page election --
			Reset();
			AddAll();

			X52Output::Page* pages[] = { &testPage1, &testPage2, &testPage3 };
			for (int i = 2; i > 0; i--)
			{
				pages[i]->Remove(true);
				Assert::IsFalse(pages[i]->IsActive() || pages[i]->IsAdded());

				auto next = i > 0 ? pages[i - 1] : nullptr;
				Assert::IsTrue(next == x52.GetActivePage());
				Assert::IsTrue(next == nullptr || !next->IsDirty());
				Assert::IsFalse(WaitEventFor(500ms));
			}
		}


		TEST_METHOD (AddRemoveFifo)
		{
			AddAll();

			for (auto pg : { &testPage1, &testPage2 })
			{
				pg->Remove();
				Assert::IsTrue(&testPage3 == x52.GetActivePage());
				Assert::IsFalse(pg->IsActive() || pg->IsAdded());
				AssertDraw(testPage3);
				Assert::IsFalse(WaitEventFor(500ms));
			}
			testPage3.Remove();
			Assert::IsTrue(nullptr == x52.GetActivePage());
			Assert::IsFalse(testPage3.IsActive() || testPage3.IsAdded());
			Assert::IsFalse(WaitEventFor(500ms));
		}



		void AddAll()
		{
			AddForeground(testPage1);
			Assert::IsFalse(WaitEventFor(500ms));
			AssertDraw(testPage1);
			Assert::IsFalse(WaitEventFor(500ms));

			AddForeground(testPage2);
			Assert::IsFalse(WaitEventFor(500ms));
			AssertDraw(testPage2);
			Assert::IsFalse(WaitEventFor(500ms));

			AddForeground(testPage3);
			Assert::IsFalse(WaitEventFor(500ms));
			AssertDraw(testPage3);
			Assert::IsFalse(WaitEventFor(500ms));
		}


		void AddBackground(X52Output::Page& pg)
		{
			X52Output::Page* const act = x52.GetActivePage();

			x52.AddPage(pg, false);
			Assert::IsTrue(pg.IsAdded() && x52.HasPage(pg.Id));
			Assert::IsFalse(pg.IsActive());
			Assert::IsTrue(act == x52.GetActivePage());
		}


		void AddForeground(X52Output::Page& pg)
		{
			x52.AddPage(pg, true);
			Assert::IsTrue(pg.IsAdded() && x52.HasPage(pg.Id));
			Assert::IsTrue(pg.IsActive());
			Assert::IsTrue(&pg == x52.GetActivePage());
		}

		
		bool WaitEventFor(Duration dur)
		{
			return x52.ProcessNextMessage(TimePoint::clock::now() + dur);
		}
	};

}
