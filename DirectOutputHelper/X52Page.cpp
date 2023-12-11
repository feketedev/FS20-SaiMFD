#include "X52Page.h"
#include "DirectOutputInstance.h"
#include "DirectOutputError.h"
#include "Saitek/DirectOutputImpl.h"
#include "Utils/Debug.h"



namespace DOHelper
{

	X52Output::Page::Page(uint32_t id) : Id { id }
	{
	}


	X52Output::Page::~Page()
    {
        if (IsAdded ())
            device->PageDestroys(*this);
    };


	void X52Output::Page::Remove()
	{
        if (IsAdded ())
			device->RemovePage(*this);
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


	void X52Output::Page::DrawLines() const
	{
        LOGIC_ASSERT (IsActive());

        Saitek::DirectOutput& lib = *device->DirectOutput().library;

        for (DWORD i = 0; i < 3; i++)
        {
            if (!isDirty[i])
                continue;

            const wchar_t* s = lines[i].data();
            DWORD        len = static_cast<DWORD>(lines[i].length());
			// NOTE: very theoretic overflow if 8GB of text -> would cause weird truncation

            SAI_ASSERT (lib.SetString(device->Handle(), Id, i, len, s));
        }
	}


} // namespace DOHelper