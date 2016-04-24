#include "border.h"
#include "window.h"
#include "keys.h"

void UpdateBorder(std::string BorderType)
{
    Assert(BorderType == "focused" || BorderType == "marked");

    kwm_border *Border = &FocusedBorder;
    int WindowID = KWMFocus.Window ? KWMFocus.Window->WID : -1;

    if(!KWMHotkeys.ActiveMode->Color.Format.empty())
        Border->Color = KWMHotkeys.ActiveMode->Color;

    if(BorderType == "marked")
    {
        WindowID = KWMScreen.MarkedWindow;
        Border = &MarkedBorder;
    }

    OpenBorder(Border);
    if(!Border->Enabled)
        CloseBorder(Border);

    if(WindowID == -1)
        ClearBorder(Border);
    else if(Border->Enabled)
        RefreshBorder(Border, WindowID);
}
