#include "X52Page.h"
#include "DirectOutputInstance.h"
#include "DirectOutputError.h"
#include "Saitek/DirectOutputImpl.h"
#include "Utils/CastUtils.h"
#include "Utils/Debug.h"



namespace DOHelper
{
	using namespace Utils::Cast;


	X52Output::Page::Page(uint32_t id) : Id { id }
	{
	}


	X52Output::Page::~Page()
    {
        if (IsAdded ())
            device->PageDestroys(*this);
    };


	void X52Output::Page::Remove(bool activateNeighbor)
	{
        if (IsAdded ())
			device->RemovePage(*this, activateNeighbor);
        DBG_ASSERT (!IsAdded());
	}


	std::wstring&   X52Output::Page::ModLine(char i)
	{
        LOGIC_ASSERT (i < 3);

		isDirty[i] = true;
        return lines[i];
	}


	std::wstring&	X52Output::Page::SetLine(char i, std::wstring text, bool allowMarquee)
	{
        LOGIC_ASSERT (i < 3);

        if (!allowMarquee && text.length() > DisplayLength)
            text.resize(DisplayLength);

		isDirty[i] = true;
		return lines[i] = std::move(text);
	}


	void X52Output::Page::Activate(TimePoint stamp)
	{
		isDirty[0] = true;
		isDirty[1] = true;
		isDirty[2] = true;
		OnActivate(stamp);
	}


	void X52Output::Page::DrawLines()
	{
        LOGIC_ASSERT (IsActive());

		bool stillActive = true;
        for (DWORD i = 0; i < 3 && stillActive; i++)
        {
            if (!isDirty[i])
                continue;

            const wchar_t* s = lines[i].data();
            DWORD        len = Practically<DWORD>(lines[i].length());
			stillActive		 = device->TrySetActivePageLine(i, len, s);
			isDirty[i] = !stillActive;
        }
	}


	bool X52Output::Page::ProbeActive() const
	{
        Saitek::DirectOutput& lib = *device->DirectOutput().library;

        DWORD   len = Practically<DWORD>(lines[0].length());
        HRESULT hr  = lib.SetString(device->Handle(), Id, 0, len, lines[0].data());

		if (hr == NotActiveError)
			return false;

		SAI_ASSERT_M (hr, "Failed to recover active page id.");

		DBG_ASSERT (SUCCEEDED(hr));
		return true;
	}


} // namespace DOHelper
