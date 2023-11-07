#pragma once

#include "DirectOutputHelper/X52Page.h"
#include <string>



namespace FSMfd::Pages
{

    class WaitSpinner final : public DOHelper::X52Output::Page {
        
        const unsigned  stepCount;
        unsigned        step = 0;
        std::wstring    status;

    public:
        const std::wstring  Title;
        const std::wstring  Bar;
        const std::wstring& Status() const  { return status; }

        WaitSpinner(std::wstring title, uint32_t id = 0, std::wstring bar = L"<-->");

        void SetStatus(std::wstring);
        void DrawAnimation();
    };


}	// namespace FSMfd::Pages
