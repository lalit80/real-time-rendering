#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// Xlib header files
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

// global variables
Display* gpDisplay = NULL;              // interface between XServer - XClient
XVisualInfo visualInfo;                 // hardware information (graphic card)
Window window;
Colormap colormap;

Bool bFullScreen = False;

int main(void)
{
    // function declarations
    void toggleFullScreen(void);
    void uninitialize(void);

    // variable declarations
    int defaultScreen;
    int defaultDepth;
    Status status;
    XSetWindowAttributes windowAttributes;
    Atom windowManagerDeleteAtom;
    XEvent event;
    Screen* screen = NULL;
    int screenWidth, screenHeight;
    KeySym keySim;
    char keys[26];                      // we need only 0th index conventionally array size is equal to no of alphabets (26 or 52)

    // Hello World related variables
    static XFontStruct* pFontStruct = NULL;
    static int winWidth, winHeight;
    static GC gc;
    XGCValues gcValues;
    XColor color;
    int strLength, strWidth, fontHeight;
    char str[] = "Hello World !!!";

    // code
    // open the connection with the XServer
    gpDisplay = XOpenDisplay(NULL);     // establish a connection (client to server)
    if (gpDisplay == NULL) {
        printf("XOpenDisplay failed to connect with server");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // create the default screen object
    defaultScreen = XDefaultScreen(gpDisplay);

    // get default depth
    defaultDepth = XDefaultDepth(gpDisplay, defaultScreen);

    memset((void*)&visualInfo, 0, sizeof(XVisualInfo));
    // get visual info
    status = XMatchVisualInfo(gpDisplay, defaultScreen, defaultDepth, TrueColor, &visualInfo);
    if (status == 0) {
        printf("XMatchVisualInfo failed");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // set window attributes
    memset((void*)&windowAttributes, 0, sizeof(XSetWindowAttributes));
    windowAttributes.border_pixel = 0;
    windowAttributes.background_pixmap = 0;
    windowAttributes.background_pixel = XBlackPixel(gpDisplay, visualInfo.screen);
    windowAttributes.colormap = XCreateColormap(gpDisplay, 
                                                    XRootWindow(gpDisplay, visualInfo.screen),
                                                    visualInfo.visual, AllocNone);
    windowAttributes.event_mask = KeyPressMask | ButtonPressMask | FocusChangeMask | StructureNotifyMask | ExposureMask;

    colormap = windowAttributes.colormap;

    // create the window
    window = XCreateWindow(gpDisplay,
                            XRootWindow(gpDisplay, visualInfo.screen),
                            0, 0, WIN_WIDTH, WIN_HEIGHT,
                            0, visualInfo.depth, InputOutput,
                            visualInfo.visual,
                            CWBorderPixel | CWBackPixel | CWEventMask | CWColormap,
                            &windowAttributes);
    if (!window) {
        printf("XCreateWindow failed");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // create atom for window manager to destroy the window
    windowManagerDeleteAtom = XInternAtom(gpDisplay, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(gpDisplay, window, &windowManagerDeleteAtom, 1);

    // set window title
    XStoreName(gpDisplay, window, "LRC : XWindow");

    // map the window to show it
    XMapWindow(gpDisplay, window);

    // centering of window
    screen = XScreenOfDisplay(gpDisplay, visualInfo.screen);
    screenWidth = XWidthOfScreen(screen);
    screenHeight = XHeightOfScreen(screen);
    XMoveWindow(gpDisplay, window, (screenWidth / 2) - (WIN_WIDTH / 2), (screenHeight / 2) - (WIN_HEIGHT / 2));

    // message loop
    while (1) {
        XNextEvent(gpDisplay, &event);
        switch (event.type) {
            case MapNotify:
                pFontStruct = XLoadQueryFont(gpDisplay, "fixed");
                break;

            case FocusIn:
                break;

            case FocusOut:
                break;

            case ConfigureNotify:
                winWidth = event.xconfigure.width;
                winHeight = event.xconfigure.height;
                break;

            case KeyPress:
                keySim = XkbKeycodeToKeysym(gpDisplay, event.xkey.keycode, 0, 0);
                switch(keySim) {
                    case XK_Escape:
                        XUnloadFont(gpDisplay, pFontStruct->fid);
                        XFreeGC(gpDisplay, gc);
                        uninitialize();
                        exit(EXIT_SUCCESS);
                        break;

                    default:
                        break;
                }
                
                // for alphabetic keyPress
                XLookupString(&event.xkey, keys, sizeof(keys), NULL, NULL);
                switch(keys[0]) {
                    case 'F':
                    case 'f':
                        if (bFullScreen == False) {
                            toggleFullScreen();
                            bFullScreen = True;
                        }
                        else {
                            toggleFullScreen();
                            bFullScreen = False;
                        }
                        break;

                    default:
                        break;
                }
                break;

            case Expose:
                // create the gc
                gc = XCreateGC(gpDisplay, window, 0, &gcValues);
                // set the font in this gc
                XSetFont(gpDisplay, gc, pFontStruct->fid);
                // get green color for our text
                XAllocNamedColor(gpDisplay, colormap, "green", &color, &color);
                // set this text color in current gc
                XSetForeground(gpDisplay, gc, color.pixel);

                // find length of string
                strLength = strlen(str);
                strWidth = XTextWidth(pFontStruct, str, strLength);
                fontHeight = pFontStruct->ascent + pFontStruct->descent;

                // draw the text
                XDrawString(gpDisplay, window, gc, (winWidth / 2 - strWidth / 2), (winHeight / 2 - fontHeight / 2), str, strLength);
                break;

            case 33:
                XUnloadFont(gpDisplay, pFontStruct->fid);
                XFreeGC(gpDisplay, gc);
                uninitialize();
                exit(EXIT_SUCCESS);
                break;
            
            default:
                break;
        }
    }

    uninitialize();
    return (0);
}

void uninitialize(void)
{
    if (window) {
        XDestroyWindow(gpDisplay, window);
    }
    if (colormap) {
        XFreeColormap(gpDisplay, colormap);
    }
    if (gpDisplay) {
        XCloseDisplay(gpDisplay);
        gpDisplay = NULL;
    }
}

void toggleFullScreen(void)
{
    Atom windowManagerNormalStateAtom = XInternAtom(gpDisplay, "_NET_WM_STATE", False);
    Atom windowManagerFullScreenStateAtom = XInternAtom(gpDisplay, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent event;
    memset((void*)&event, 0, sizeof(XEvent));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = windowManagerNormalStateAtom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = bFullScreen ? 0 : 1;
    event.xclient.data.l[1] = windowManagerFullScreenStateAtom;

    // send above event to xserver
    XSendEvent(gpDisplay,
                XRootWindow(gpDisplay, visualInfo.screen),
                False, SubstructureNotifyMask, &event);
}
