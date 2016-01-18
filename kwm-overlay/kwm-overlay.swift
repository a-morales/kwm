import Foundation
import AppKit

func parseFrame(args: Array<String>) -> NSRect
{
    var frameHash = [String: Int]()
    frameHash["w"] = 0
    frameHash["h"] = 0
    frameHash["x"] = 0
    frameHash["y"] = 0

    let windowId = Int(args[0])

    if (windowId != nil) {
        let option = CGWindowListOption(arrayLiteral: CGWindowListOption.ExcludeDesktopElements, CGWindowListOption.OptionOnScreenOnly)
        let relativeToWindow = CGWindowID(windowId!)
        let windowInfoListRef = CGWindowListCopyWindowInfo(option, relativeToWindow)
        let windowInfoList = windowInfoListRef as NSArray? as? [[String: AnyObject]]
        let scrns: Array<NSScreen> = NSScreen.screens()!
        let scrn: NSScreen = scrns[0]
        let scrnHeight: Int = Int(scrn.frame.size.height)

        for windowInfo in windowInfoList! {
            let windowNumber = windowInfo["kCGWindowNumber"]! as! Int
            if (windowNumber == windowId) {
                let windowBounds = windowInfo["kCGWindowBounds"]! as! [String : Int]
                frameHash["w"] = windowBounds["Width"]
                frameHash["h"] = windowBounds["Height"]!
                frameHash["x"] = windowBounds["X"]
                frameHash["y"] = scrnHeight - (windowBounds["Y"]! + windowBounds["Height"]!)
                break
            }
        }
    }

    for arg in args {
        if ((arg.characters.indexOf(":")) != nil) {
            let pair: Array = arg.componentsSeparatedByString(":")
            let key: String = pair[0]
            let val: String = pair[1]

            switch key {
            case "x":
                frameHash["x"] = Int(val)
                break
            case "y":
                frameHash["y"] = Int(val)
                break
            case "w", "width":
                frameHash["w"] = Int(val)
                break
            case "h", "height":
                frameHash["h"] = Int(val)
                break
            default:
                break
            }
        }
    }

    return NSRect(x: frameHash["x"]! - 1, y: frameHash["y"]! - 1, width: frameHash["w"]! + 2, height: frameHash["h"]! + 2)
}

func parseColor(args: Array<String>) -> NSColor
{
    var colorHash = [String: CGFloat]()
    colorHash["r"] = 1
    colorHash["g"] = 0
    colorHash["b"] = 0
    colorHash["a"] = 1

    for arg in args {
        if ((arg.characters.indexOf(":")) != nil) {
            let pair: Array = arg.componentsSeparatedByString(":")
            let key: String = pair[0]
            let val: String = pair[1]
            switch key {
            case "r", "red":
                colorHash["r"] = CGFloat(Float(val)!)
                break
            case "b", "blue":
                colorHash["b"] = CGFloat(Float(val)!)
                break
            case "g", "green":
                colorHash["g"] = CGFloat(Float(val)!)
                break
            case "a", "alpha":
                colorHash["a"] = CGFloat(Float(val)!)
                break
            default:
                break
            }
        }
    }

    return NSColor(red: colorHash["r"]!, green: colorHash["g"]!, blue: colorHash["b"]!, alpha: colorHash["a"]!)
}

func parseWidth(args: Array<String>) -> CGFloat
{
    var widthVal: CGFloat = 4

    for arg in args {
        if ((arg.characters.indexOf(":")) != nil) {
            let pair: Array = arg.componentsSeparatedByString(":")
            let key: String = pair[0]
            let val: String = pair[1]
            switch key {
            case "s", "stroke":
                widthVal = CGFloat(Float(val)!)
                break
            default:
                break
            }
        }
    }

    return widthVal
}


class OverlayView: NSView
{
    let borderColor: NSColor
    let lineWidth: CGFloat
    init(frame: NSRect, color: NSColor, width: CGFloat)
    {
        borderColor = color
        lineWidth = width
        super.init(frame: frame)
    }

    required init?(coder: NSCoder)
    {
        fatalError("init(coder:) has not been implemented")
    }

    var colorclear = NSColor.clearColor()

    override func drawRect(rect: NSRect)
    {
        colorclear.setFill()

        let bpath:NSBezierPath = NSBezierPath(rect: rect)

        borderColor.set()
        bpath.lineWidth = lineWidth
        bpath.stroke()
    }
}

class OverlayController: NSObject, NSApplicationDelegate
{

    let window = NSWindow()

    func showOverlayView(args: Array<String>)
    {
        let overlayFrame = parseFrame(args)
        let overlayColor = parseColor(args)
        let overlayWidth = parseWidth(args)
        let overlayView = OverlayView(frame: overlayFrame, color: overlayColor, width: overlayWidth)

        window.contentView = overlayView
        window.setFrame(NSRectToCGRect(overlayFrame), display: true)
    }

    func applicationDidFinishLaunching(aNotification: NSNotification)
    {
        window.opaque = false
        window.backgroundColor = NSColor.clearColor()
        window.titleVisibility = NSWindowTitleVisibility.Hidden
        window.titlebarAppearsTransparent = true
        window.ignoresMouseEvents = true
        window.styleMask |= NSFullSizeContentViewWindowMask
        window.level = Int(CGWindowLevelForKey(.FloatingWindowLevelKey))
        window.hasShadow = false
        window.collectionBehavior = NSWindowCollectionBehavior.CanJoinAllSpaces
        window.makeKeyAndOrderFront(self)
        self.listenToStdIn()

    }

    func listenToStdIn() -> NSObjectProtocol
    {
        let outHandle = NSFileHandle.fileHandleWithStandardInput()
        outHandle.waitForDataInBackgroundAndNotify()

        var stdListen: NSObjectProtocol!
        stdListen = NSNotificationCenter.defaultCenter().addObserverForName(NSFileHandleDataAvailableNotification, object: outHandle, queue: nil) {
            notification -> Void in
            let data = outHandle.availableData
            if (data.length > 0) {
                if let str = NSString(data: data, encoding: NSUTF8StringEncoding) as String? {
                    let trimmedString = str.stringByTrimmingCharactersInSet(
                        NSCharacterSet.whitespaceAndNewlineCharacterSet()
                    )

                    if (trimmedString == "clear") {
                        self.window.contentView = nil
                    } else {
                        let args = trimmedString.componentsSeparatedByString(" ")
                        self.showOverlayView(args)
                    }

                    outHandle.waitForDataInBackgroundAndNotify()
                } else {
                    NSNotificationCenter.defaultCenter().removeObserver(stdListen)
                }

            }
        }

        return stdListen
    }

}

let app = NSApplication.sharedApplication()
let overlayController = OverlayController()

app.delegate = overlayController
app.run()
