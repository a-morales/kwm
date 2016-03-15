#ifndef BORDER_H
#define BORDER_H

#include "types.h"
#include "window.h"

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
    AXUIElementRef WindowRef;
    window_info *Window = GetWindowByID(WindowID);
    if(GetWindowRef(Window, &WindowRef))
    {
        CGPoint WindowPos = GetWindowPos(WindowRef);
        CGSize WindowSize = GetWindowSize(WindowRef);
        std::string Command = "x:" + std::to_string(WindowPos.x) + \
                              " y:" + std::to_string(WindowPos.y) + \
                              " w:" + std::to_string(WindowSize.width) + \
                              " h:" + std::to_string(WindowSize.height) + \
                              " " + Border->Color.Format + \
                              " s:" + std::to_string(Border->Width);

        Command += Border->Radius != -1 ? " rad:" + std::to_string(Border->Radius) : "";

        std::cout << Command << std::endl;
        fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
        fflush(Border->Handle);
    }
}

void UpdateBorder(std::string BorderType);

#endif
