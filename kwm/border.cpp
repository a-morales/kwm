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
    Assert(WindowID != -1, "ConvertHexToRGBAString()")

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

void ClearBorder(kwm_border *Border)
{
    if(Border->Handle)
    {
        std::string Command = "clear";
        fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
        fflush(Border->Handle);
    }
}

void OpenBorder(kwm_border *Border)
{
    if(Border->Enabled && !Border->Handle)
    {
        std::string OverlayBin = KWMPath.FilePath + "/kwm-overlay";
        Border->Handle = popen(OverlayBin.c_str(), "w");
        if(!Border->Handle)
            Border->Enabled = false;
    }
}

void RefreshBorder(kwm_border *Border, int WindowID)
{
    std::string Command = ConvertHexToRGBAString(WindowID, Border->Color, Border->Width);
    fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
    fflush(Border->Handle);
}

void CloseBorder(kwm_border *Border)
{
    if(Border->Handle)
    {
        std::string Terminate = "quit";
        fwrite(Terminate.c_str(), Terminate.size(), 1, Border->Handle);
        fflush(Border->Handle);
        pclose(Border->Handle);
        Border->Handle = NULL;
    }
}

void UpdateBorder(std::string BorderType)
{
    Assert(BorderType == "focused" ||
           BorderType == "marked", "UpdateBorder()")

    kwm_border *Border = &FocusedBorder;
    int WindowID = GetFocusedWindowID();

    if(BorderType == "focused" &&
       PrefixBorder.Enabled &&
       KWMHotkeys.Prefix.Active)
    {
        Border = &PrefixBorder;
    }
    else if(BorderType == "marked")
    {
        WindowID = KWMScreen.MarkedWindow;
        Border = &MarkedBorder;
    }

    OpenBorder(Border);
    if(!Border->Enabled)
        CloseBorder(Border);

    if(Border->Enabled)
    {
        if(WindowID == -1)
            ClearBorder(Border);
        else
            RefreshBorder(Border, WindowID);
    }
}
