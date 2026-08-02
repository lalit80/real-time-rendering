#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// Xlib header files
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

// open header files
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

// global variables
Display* gpDisplay = NULL;              // interface between XServer - XClient
XVisualInfo* visualInfo = NULL;                 // hardware information (graphic card)
Window window;
Colormap colormap;

Bool bFullScreen = False;
Bool bActiveWindow = False;

// opengl related variables
GLXContext glxContext = NULL;

// file io
char gszLogFileName[] = "log.txt";
FILE* gpFile = NULL;

int main(void)
{
    // function declarations
    void toggleFullScreen(void);
    int initialize(void);
    void resize(int, int);
    void display(void);
    void update(void);
    void uninitialize(void);

    // variable declarations
    int defaultScreen;
    int defaultDepth;
    XSetWindowAttributes windowAttributes;
    Atom windowManagerDeleteAtom;
    XEvent event;
    Screen* screen = NULL;
    int screenWidth, screenHeight;
    KeySym keySim;
    char keys[26];

    int framebufferAttributes[] = { GLX_DOUBLEBUFFER, GLX_RGBA,
                                    GLX_RED_SIZE, 8,
                                    GLX_GREEN_SIZE, 8,
                                    GLX_BLUE_SIZE, 8,
                                    GLX_ALPHA_SIZE, 8, None};
    Bool bDone = False;

    gpFile = fopen(gszLogFileName, "w");
    if(gpFile == NULL) {
        printf("Log file creation failed");
        exit(EXIT_FAILURE);
    }
    else {
        fprintf(gpFile, "Program started successfully\n");
    }

    // code
    // open the connection with the XServer
    gpDisplay = XOpenDisplay(NULL);     // establish a connection (client to server)
    if (gpDisplay == NULL) {
        fprintf(gpFile, "XOpenDisplay failed to connect with server\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // create the default screen object
    defaultScreen = XDefaultScreen(gpDisplay);

    // get default depth
    defaultDepth = XDefaultDepth(gpDisplay, defaultScreen);

    visualInfo = glXChooseVisual(gpDisplay, defaultScreen, framebufferAttributes);
    if (visualInfo == NULL) {
        fprintf(gpFile, "glXChooseVisual failed\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // set window attributes
    memset((void*)&windowAttributes, 0, sizeof(XSetWindowAttributes));
    windowAttributes.border_pixel = 0;
    windowAttributes.background_pixmap = 0;
    windowAttributes.background_pixel = XBlackPixel(gpDisplay, visualInfo->screen);
    windowAttributes.colormap = XCreateColormap(gpDisplay, 
                                                    XRootWindow(gpDisplay, visualInfo->screen),
                                                    visualInfo->visual, AllocNone);
    windowAttributes.event_mask = KeyPressMask | ButtonPressMask | FocusChangeMask | StructureNotifyMask | ExposureMask;

    colormap = windowAttributes.colormap;

    // create the window
    window = XCreateWindow(gpDisplay,
                            XRootWindow(gpDisplay, visualInfo->screen),
                            0, 0, WIN_WIDTH, WIN_HEIGHT,
                            0, visualInfo->depth, InputOutput,
                            visualInfo->visual,
                            CWBorderPixel | CWBackPixel | CWEventMask | CWColormap,
                            &windowAttributes);
    if (!window) {
        fprintf(gpFile, "XCreateWindow failed\n");
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
    screen = XScreenOfDisplay(gpDisplay, visualInfo->screen);
    screenWidth = XWidthOfScreen(screen);
    screenHeight = XHeightOfScreen(screen);
    XMoveWindow(gpDisplay, window, (screenWidth / 2) - (WIN_WIDTH / 2), (screenHeight / 2) - (WIN_HEIGHT / 2));

    // initialize
    int iResult = initialize();
    if (iResult == -1) {
        fprintf(gpFile, "initialize failed\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // game loop
    while (bDone == False) {
        while (XPending(gpDisplay)) {
            XNextEvent(gpDisplay, &event);
            switch (event.type) {
                case MapNotify:
                    break;

                case FocusIn:
                    bActiveWindow = True;
                    break;

                case FocusOut:
                    bActiveWindow = False;
                    break;

                case ConfigureNotify:
                    resize(event.xconfigure.width, event.xconfigure.height);
                    break;

                case KeyPress:
                    keySim = XkbKeycodeToKeysym(gpDisplay, event.xkey.keycode, 0, 0);
                    switch(keySim) {
                        case XK_Escape:
                            bDone = True;
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
                    break;

                case 33:
                    bDone = True;
                    break;
                
                default:
                    break;
            }
        }

        // rendering
        if (bActiveWindow == True) {
            display();
            update();
        }
    }

    uninitialize();
    return (0);
}

int initialize(void)
{
    void printGLInfo(void);
    // code
    glxContext = glXCreateContext(gpDisplay, visualInfo, NULL, True);
    if (glxContext == NULL) {
        fprintf(gpFile, "glxCreateContext failed\n");
        return (-1);
    }
    glXMakeCurrent(gpDisplay, window, glxContext);

    printGLInfo();
    glClearColor(0, 0, 0, 1);

    return 0;
}

void resize(int width, int height)
{
    // code
    if (height <= 0) {
        height = 1;
    }
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // set matrix projection mode
    glMatrixMode(GL_PROJECTION);

    // set matrix projection mode
    glMatrixMode(GL_PROJECTION);

    // set to identity matrix
    glLoadIdentity();

    // do perspective projection
    gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void display(void)
{
    // code

    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT);

    // set matrix to model view mode
    glMatrixMode(GL_MODELVIEW);

    // set to identity matrix
    glLoadIdentity();

    // translate triangle backwards by z
    glTranslatef(0.0f, 0.0f, -3.0f);

    glBegin(GL_QUADS);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glVertex3f(-1.0f, 1.0f, 0.0f);
		glVertex3f(-1.0f, -1.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();

    // swap the buffers
    glXSwapBuffers(gpDisplay, window);
}

void update(void)
{
    // code
}

void uninitialize(void)
{
    GLXContext currentContext = glXGetCurrentContext();
    if (currentContext && currentContext == glxContext) {
        glXMakeCurrent(gpDisplay, 0, 0);
    }
    if (glxContext) {
        glXDestroyContext(gpDisplay, glxContext);
        glxContext = NULL;
    }
    if (visualInfo) {
        free(visualInfo);
        visualInfo = NULL;
    }   
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
    // close the file
    if(gpFile) {
        fprintf(gpFile, "prorgam terminated successfully\n");
        fclose(gpFile);
        gpFile = NULL;
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
                XRootWindow(gpDisplay, visualInfo->screen),
                False, SubstructureNotifyMask, &event);
}

void printGLInfo(void)
{
    // code
    // print opengl information
    fprintf(gpFile, "OPENGL INFORMATION\n");
    fprintf(gpFile, "------------------\n");
    fprintf(gpFile, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version : %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "------------------\n");
}
