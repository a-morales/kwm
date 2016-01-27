#include "border.h"

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_path KWMPath;
extern kwm_border KWMBorder;
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

void ClearFocusedBorder()
{
    if(KWMBorder.FHandle)
    {
        std::string Border = "clear";
        fwrite(Border.c_str(), Border.size(), 1, KWMBorder.FHandle);
        fflush(KWMBorder.FHandle);
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

void UpdateBorder(std::string Border)
{
    if(Border == "focused")
    {
        if(KWMBorder.FEnabled)
        {
            if(KWMFocus.Window && KWMFocus.Window->Layer == 0)
            {
                if(!KWMBorder.FHandle)
                {
                    std::string OverlayBin = KWMPath.FilePath + "/kwm-overlay";
                    KWMBorder.FHandle = popen(OverlayBin.c_str(), "w");
                    if(KWMBorder.FHandle == NULL)
                    {
                        KWMBorder.FEnabled = false;
                        return;
                    }
                }

                DEBUG("UpdateFocusedBorder()")
                KWMBorder.FTime = PerformUpdateBorderTimer(KWMBorder.FTime);
                int BorderWidth = KWMBorder.FWidth;
                unsigned int BorderColor = KWMBorder.FColor;
                if(KWMBorder.HEnabled && KWMHotkeys.Prefix.Active)
                {
                    BorderWidth = KWMBorder.HWidth;
                    BorderColor = KWMBorder.HColor;
                }
                std::string Border = ConvertHexToRGBAString(KWMFocus.Window->WID, BorderColor, BorderWidth);
                fwrite(Border.c_str(), Border.size(), 1, KWMBorder.FHandle);
                fflush(KWMBorder.FHandle);
            }
        }
        else
        {
            if(KWMBorder.FHandle)
            {
                std::string Terminate = "quit";
                fwrite(Terminate.c_str(), Terminate.size(), 1, KWMBorder.FHandle);
                fflush(KWMBorder.FHandle);
                pclose(KWMBorder.FHandle);
                KWMBorder.FHandle = NULL;
            }
        }
    }
    else if(Border == "marked")
    {
        if(KWMBorder.MEnabled)
        {
            if(!KWMBorder.MHandle)
            {
                std::string OverlayBin = KWMPath.FilePath + "/kwm-overlay";
                KWMBorder.MHandle = popen(OverlayBin.c_str(), "w");
                if(KWMBorder.MHandle == NULL)
                {
                    KWMBorder.MEnabled = false;
                    return;
                }
            }

            if(KWMScreen.MarkedWindow == -1)
            {
                std::string Border = "clear";
                fwrite(Border.c_str(), Border.size(), 1, KWMBorder.MHandle);
                fflush(KWMBorder.MHandle);
            }
            else
            {
                DEBUG("UpdateMarkedBorder()")
                KWMBorder.MTime = PerformUpdateBorderTimer(KWMBorder.MTime);
                std::string Border = ConvertHexToRGBAString(KWMScreen.MarkedWindow, KWMBorder.MColor, KWMBorder.MWidth);
                fwrite(Border.c_str(), Border.size(), 1, KWMBorder.MHandle);
                fflush(KWMBorder.MHandle);
            }
        }
        else
        {
            if(KWMBorder.MHandle)
            {
                std::string Terminate = "quit";
                fwrite(Terminate.c_str(), Terminate.size(), 1, KWMBorder.MHandle);
                fflush(KWMBorder.MHandle);
                pclose(KWMBorder.MHandle);
                KWMBorder.MHandle = NULL;
            }
        }
    }
}
