#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// Xlib header files
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

// global variables
Display* gpDisplay = NULL;              // interface between XServer - XClient
XVisualInfo visualInfo;                 // hardware information (graphic card)
Window window;
Colormap colormap;

int main(void)
{
    // function declarations
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
            case 33:
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
