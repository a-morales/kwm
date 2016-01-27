#include "border.h"
#include "window.h"

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_path KWMPath;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;
extern kwm_border PrefixBorder;
extern kwm_hotkeys KWMHotkeys;

std::string ConvertHexToRGBAString(int WindowID, int Color, int Width)
{
    short r = (Color >> 16) & 0xff;
    short g = (Color >> 8) & 0xff;
    short b = (Color >> 0) & 0xff;
    short a = (Color >> 24) & 0xff;

    std::string rs = std::to_string((double)r/255);
    std::string gs = std::to_string((double)g/255);
    std::string bs = std::to_string((double)b/255);
    std::string as = std::to_string((double)a/255);

    return std::to_string(WindowID) + " r:" + rs + " g:" + gs + " b:" + bs + " a:" + as + " s:" + std::to_string(Width);
}

void ClearBorder(kwm_border &Border)
{
    if(Border.Handle)
    {
        std::string Command = "clear";
        fwrite(Command.c_str(), Command.size(), 1, Border.Handle);
        fflush(Border.Handle);
    }
}

kwm_time_point PerformUpdateBorderTimer(kwm_time_point Time)
{
    kwm_time_point NewBorderTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> Diff = NewBorderTime - Time;
    while(Diff.count() < 0.10)
    {
        NewBorderTime = std::chrono::steady_clock::now();
        Diff = NewBorderTime - Time;
    }

    return NewBorderTime;
}

void CloseBorder(kwm_border &Border)
{
    if(Border.Handle)
    {
        std::string Terminate = "quit";
        fwrite(Terminate.c_str(), Terminate.size(), 1, Border.Handle);
        fflush(Border.Handle);
        pclose(Border.Handle);
        Border.Handle = NULL;
    }
}

void UpdateBorder(std::string BorderType)
{
    kwm_border *Border;
    int WindowID;
    if(BorderType == "focused")
    {
        WindowID = GetFocusedWindowID();
        if(PrefixBorder.Enabled && KWMHotkeys.Prefix.Active)
            Border = &PrefixBorder;
        else
            Border = &FocusedBorder;
    }
    else if(BorderType == "marked")
    {
        WindowID = KWMScreen.MarkedWindow;
        Border = &MarkedBorder;
    }
    else
        return;

    if(Border->Enabled && WindowID != -1)
    {
        if(!Border->Handle)
        {
            std::string OverlayBin = KWMPath.FilePath + "/kwm-overlay";
            Border->Handle = popen(OverlayBin.c_str(), "w");
            if(Border->Handle == NULL)
            {
                Border->Enabled = false;
                return;
            }
        }

        DEBUG("UpdateBorder()")
        Border->Time = PerformUpdateBorderTimer(Border->Time);
        std::string Command = ConvertHexToRGBAString(WindowID, Border->Color, Border->Width);
        fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
        fflush(Border->Handle);
    }
    else if(WindowID == -1)
        ClearBorder(*Border);
    else
        CloseBorder(*Border);
}
