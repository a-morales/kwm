#include "border.h"
#include "window.h"
#include "keys.h"
#include "axlib/axlib.h"

extern ax_window *MarkedWindow;

void UpdateBorder(std::string BorderType)
{
    Assert(BorderType == "focused" || BorderType == "marked");

    kwm_border *Border = &FocusedBorder;
    ax_application *Application = AXLibGetFocusedApplication();
    ax_window *Window = Application ? Application->Focus : NULL;

    if(!KWMHotkeys.ActiveMode->Color.Format.empty())
        Border->Color = KWMHotkeys.ActiveMode->Color;

    if(BorderType == "marked")
    {
        Window = MarkedWindow;
        Border = &MarkedBorder;
    }

    OpenBorder(Border);
    if(!Border->Enabled)
        CloseBorder(Border);

    if(!Window)
        ClearBorder(Border);
    else if(Border->Enabled)
        RefreshBorder(Border, Window);
}
