#include "border.h"
#include "window.h"

void UpdateBorder(std::string BorderType)
{
    Assert(BorderType == "focused" || BorderType == "marked")

    kwm_border *Border = &FocusedBorder;
    int WindowID = GetFocusedWindowID();

    if(BorderType == "focused" &&
       FocusedBorder.Enabled &&
       PrefixBorder.Enabled &&
       KWMHotkeys.Prefix.Active)
    {
        Border = &PrefixBorder;
        Border->Handle = FocusedBorder.Handle;
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
