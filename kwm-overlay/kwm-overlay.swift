import Foundation
import AppKit

func trim(str: String) -> String {
    return str.stringByTrimmingCharactersInSet(
        NSCharacterSet.whitespaceAndNewlineCharacterSet()
    )

}

func parseFrame(args: Array<String>, strokeWidth: CGFloat) -> NSRect
{
    var frameHash = [String: CGFloat]()
    frameHash["w"] = 0
    frameHash["h"] = 0
    frameHash["x"] = 0
    frameHash["y"] = 0

    let scrns: Array<NSScreen> = NSScreen.screens()!
    let scrn: NSScreen = scrns[0]
    let scrnHeight: CGFloat = scrn.frame.size.height

    for arg in args {
        if ((arg.characters.indexOf(":")) != nil) {
            let pair: Array = arg.componentsSeparatedByString(":")
            let key: String = pair[0]
            let val: String = trim(pair[1])
            let intVal = Float(val)
            if (intVal == nil) {
                continue
            }

            switch key {
            case "x":
                frameHash["x"] = CGFloat(intVal!)
                break
            case "y":
                frameHash["y"] = CGFloat(intVal!)
                break
            case "w", "width":
                frameHash["w"] = CGFloat(intVal!)
                break
            case "h", "height":
                frameHash["h"] = CGFloat(intVal!)
                break
            default:
                break
            }
        }
    }

    frameHash["y"] = scrnHeight - (frameHash["y"]! + frameHash["h"]!)

    let computedFrame = NSRect(x: frameHash["x"]!,
                               y: frameHash["y"]!,
                           width: frameHash["w"]!,
                          height: frameHash["h"]!)

    return CGRectInset(computedFrame, -strokeWidth, -strokeWidth)
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
            let val: String = trim(pair[1])
            let floatVal = Float(val)
            if (floatVal == nil) {
                continue
            }

            switch key {
            case "r", "red":
                colorHash["r"] = CGFloat(floatVal!)
                break
            case "b", "blue":
                colorHash["b"] = CGFloat(floatVal!)
                break
            case "g", "green":
                colorHash["g"] = CGFloat(floatVal!)
                break
            case "a", "alpha":
                colorHash["a"] = CGFloat(floatVal!)
                break
            default:
                break
            }
        }
    }

    return NSColor(red: colorHash["r"]!, green: colorHash["g"]!, blue: colorHash["b"]!, alpha: colorHash["a"]!)
}

func parseStroke(args: Array<String>) -> [String: CGFloat]
{
    var strokeHash = [String: CGFloat]()
    strokeHash["size"] = CGFloat(4)

    for arg in args {
        if ((arg.characters.indexOf(":")) != nil) {
            let pair: Array = arg.componentsSeparatedByString(":")
            let key: String = pair[0]
            let val: String = trim(pair[1])
            let floatVal = Float(val)
            if (floatVal == nil) {
                continue
            }

            switch key {
            case "s", "stroke":
                strokeHash["size"] = CGFloat(floatVal!)
                break
            case "rad":
                strokeHash["rad"] = CGFloat(floatVal!)
                break
            default:
                break
            }
        }
    }

    if (strokeHash["rad"] == nil) {
        strokeHash["rad"] = strokeHash["size"]! + 4
    }

    return strokeHash
}


class OverlayView: NSView
{
    let borderColor: NSColor
    let lineWidth: CGFloat
    let lineRadius: CGFloat
    init(frame: NSRect, color: NSColor, width: CGFloat, radius: CGFloat)
    {
        borderColor = color
        lineWidth = width
        lineRadius = radius
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

        let bpath:NSBezierPath = NSBezierPath(roundedRect: CGRectInset(rect, lineWidth/2, lineWidth/2), xRadius:lineRadius, yRadius:lineRadius)

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
        let overlayColor = parseColor(args)
        let overlayStroke = parseStroke(args)
        let overlayFrame = parseFrame(args, strokeWidth: overlayStroke["size"]!)

        if(NSIsEmptyRect(overlayFrame)) {
            window.contentView = nil
            return
        }

        let overlayView = OverlayView(frame: overlayFrame, color: overlayColor, width: overlayStroke["size"]!, radius: overlayStroke["rad"]!)

        window.contentView = overlayView
        window.setFrame(NSRectToCGRect(overlayFrame), display: true)
    }

    func applicationDidFinishLaunching(aNotification: NSNotification)
    {
        window.opaque = false
        window.backgroundColor = NSColor.clearColor()
        window.styleMask = NSBorderlessWindowMask
        window.ignoresMouseEvents = true
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
                    let trimmedString = trim(str);

                    if (trimmedString == "clear") {
                        self.window.contentView = nil
                    } else if (trimmedString == "quit") {
                        exit(0)
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
