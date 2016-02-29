#ifndef BORDER_H
#define BORDER_H

#include "types.h"

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_path KWMPath;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;
extern kwm_border PrefixBorder;
extern kwm_hotkeys KWMHotkeys;

inline void
OpenBorder(kwm_border *Border)
{
    if(Border->Enabled && !Border->Handle)
    {
        static std::string OverlayBin = KWMPath.FilePath + "/kwm-overlay";
        Border->Handle = popen(OverlayBin.c_str(), "w");
        if(!Border->Handle)
            Border->Enabled = false;
    }
}

inline void
CloseBorder(kwm_border *Border)
{
    if(Border->Handle)
    {
        static std::string Terminate = "quit";
        fwrite(Terminate.c_str(), Terminate.size(), 1, Border->Handle);
        fflush(Border->Handle);
        pclose(Border->Handle);
        Border->Handle = NULL;
    }
}

inline void
ClearBorder(kwm_border *Border)
{
    if(Border->Handle)
    {
        static std::string Command = "clear";
        fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
        fflush(Border->Handle);
    }
}

inline void
RefreshBorder(kwm_border *Border, int WindowID)
{
    std::string Command = std::to_string(WindowID) + \
                          " " + Border->Color.Format + \
                          " s:" + std::to_string(Border->Width);

    Command += Border->Radius != -1 ? " rad:" + std::to_string(Border->Radius) : "";

    fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
    fflush(Border->Handle);
}

void UpdateBorder(std::string BorderType);

#endif
