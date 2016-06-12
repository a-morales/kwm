#include "border.h"
#include "window.h"
#include "keys.h"

extern  ax_application *FocusedApplication;

void UpdateBorder(std::string BorderType)
{
    Assert(BorderType == "focused" || BorderType == "marked");

    kwm_border *Border = &FocusedBorder;
    unsigned int WindowID = FocusedApplication && FocusedApplication->Focus ? FocusedApplication->Focus->ID : 0;

    if(!KWMHotkeys.ActiveMode->Color.Format.empty())
        Border->Color = KWMHotkeys.ActiveMode->Color;

    if(BorderType == "marked")
    {
        WindowID = KWMScreen.MarkedWindow.WID;
        Border = &MarkedBorder;
    }

    OpenBorder(Border);
    if(!Border->Enabled)
        CloseBorder(Border);

    if(WindowID == 0)
        ClearBorder(Border);
    else if(Border->Enabled)
        RefreshBorder(Border, FocusedApplication->Focus);
}
