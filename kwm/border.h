#ifndef BORDER_H
#define BORDER_H

#include "types.h"
#include "window.h"

#include "axlib/axlib.h"

extern kwm_path KWMPath;

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
        static std::string Terminate = "quit\n";
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
        static std::string Command = "clear\n";
        fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
        fflush(Border->Handle);
    }
}

inline void
RefreshBorder(kwm_border *Border, ax_window *Window)
{
    std::string Command = "x:" + std::to_string(Window->Position.x) + \
                          " y:" + std::to_string(Window->Position.y) + \
                          " w:" + std::to_string(Window->Size.width) + \
                          " h:" + std::to_string(Window->Size.height) + \
                          " " + Border->Color.Format + \
                          " s:" + std::to_string(Border->Width);

    Command += (Border->Radius != -1 ? " rad:" + std::to_string(Border->Radius) : "") + "\n";
    fwrite(Command.c_str(), Command.size(), 1, Border->Handle);
    fflush(Border->Handle);
}

void UpdateBorder(std::string BorderType);

#endif
