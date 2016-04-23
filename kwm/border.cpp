#include "border.h"
#include "window.h"

bool DoesModeHaveCustomBorder(std::string Mode)
{
    std::map<std::string, color>::iterator It = FocusedBorder.ModeColors.find(Mode);
    return It != FocusedBorder.ModeColors.end();
}

void UpdateBorder(std::string BorderType)
{
    Assert(BorderType == "focused" || BorderType == "marked");

    kwm_border *Border = &FocusedBorder;
    int WindowID = KWMFocus.Window ? KWMFocus.Window->WID : -1;

    if(DoesModeHaveCustomBorder(KWMHotkeys.ActiveMode))
        Border->Color = FocusedBorder.ModeColors[KWMHotkeys.ActiveMode];
    else
        Border->Color = FocusedBorder.ModeColors["default"];

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
